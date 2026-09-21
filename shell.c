#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 64

static volatile sig_atomic_t sigint_received = 0;

static int tokenize(const char *input, char **argv, size_t argv_capacity,
                    char *output) {
  const char *read = input;
  char *write = output;
  int argc = 0;

  while (*read != '\0') {
    while (isspace((unsigned char)*read)) {
      read++;
    }

    if (*read == '\0') {
      break;
    }

    if ((size_t)argc >= argv_capacity - 1) {
      fprintf(stderr, "shell: too many arguments\n");
      return -1;
    }

    /*
     * Redirection operators are always separate tokens.
     */
    if (*read == '<' || *read == '>') {
      argv[argc++] = write;
      *write++ = *read++;

      if (write[-1] == '>' && *read == '>') {
        *write++ = *read++;
      }

      *write++ = '\0';
      continue;
    }

    /*
     * Start a normal argument.
     */
    argv[argc++] = write;

    int in_single_quote = 0;
    int in_double_quote = 0;

    while (*read != '\0') {
      char current = *read;

      if (!in_single_quote && current == '"') {
        in_double_quote = !in_double_quote;
        read++;
        continue;
      }

      if (!in_double_quote && current == '\'') {
        in_single_quote = !in_single_quote;
        read++;
        continue;
      }

      if (!in_single_quote && current == '\\') {
        read++;

        if (*read == '\0') {
          fprintf(stderr, "shell: trailing backslash\n");
          return -1;
        }

        *write++ = *read++;
        continue;
      }

      if (!in_single_quote && !in_double_quote &&
          isspace((unsigned char)current)) {
        read++; // Consume the delimiter before overwriting it.
        break;
      }

      if (!in_single_quote && !in_double_quote &&
          (current == '<' || current == '>')) {
        break;
      }

      *write++ = *read++;
    }

    if (in_single_quote || in_double_quote) {
      fprintf(stderr, "shell: unmatched quote\n");
      return -1;
    }

    *write++ = '\0';
  }

  argv[argc] = NULL;
  return argc;
}

static void handle_sigint(int signal_number) {
  (void)signal_number;
  sigint_received = 1;
}

static void print_prompt(void) {
  char *cwd = getcwd(NULL, 0);

  if (cwd == NULL) {
    perror("getcwd");
    return;
  }

  printf("[%s]$ ", cwd);
  fflush(stdout);

  free(cwd);
}

static char *trim_whitespace(char *str) {
  char *end;

  while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
    str++;
  }

  if (*str == '\0') {
    return str;
  }

  end = str + strlen(str) - 1;
  while (end > str &&
         (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
    *end = '\0';
    end--;
  }

  return str;
}

static int run_builtin(char **argv) {
  if (strcmp(argv[0], "exit") == 0) {
    exit(0);
  }

  if (strcmp(argv[0], "cd") == 0) {
    if (argv[1] == NULL || strcmp(argv[1], "~") == 0) {
      argv[1] = getenv("HOME");
    }

    if (argv[1] == NULL) {
      fprintf(stderr, "cd: HOME not set\n");
      return 1;
    }

    if (chdir(argv[1]) != 0) {
      perror("cd");
      return 1;
    }

    return 1;
  }

  if (strcmp(argv[0], "pwd") == 0) {
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("pwd");
      return 1;
    }
    printf("%s\n", cwd);
    return 1;
  }

  return 0;
}

static int is_redirection_operator(const char *token) {
  return strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 ||
         strcmp(token, "<") == 0;
}

static int apply_redirections(char **argv) {
  int read_index = 0;
  int write_index = 0;

  while (argv[read_index] != NULL) {
    char *token = argv[read_index];

    if (!is_redirection_operator(token)) {
      argv[write_index++] = argv[read_index++];
      continue;
    }

    char *operator = argv[read_index];
    char *filename = argv[read_index + 1];

    if (filename == NULL || is_redirection_operator(filename)) {
      fprintf(stderr, "shell: redirection requires a filename\n");
      return -1;
    }

    int fd;

    if (strcmp(operator, "<") == 0) {
      fd = open(filename, O_RDONLY);

      if (fd == -1) {
        perror(filename);
        return -1;
      }

      if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
      }
    } else if (strcmp(operator, ">") == 0) {
      fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

      if (fd == -1) {
        perror(filename);
        return -1;
      }

      if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
      }
    } else {
      fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);

      if (fd == -1) {
        perror(filename);
        return -1;
      }

      if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return -1;
      }
    }

    close(fd);

    /*
     * Skip both the operator and filename.
     * They must not be passed to execvp().
     */
    read_index += 2;
  }

  argv[write_index] = NULL;
  return 0;
}

static int is_builtin_command(const char *command) {
  return strcmp(command, "exit") == 0 || strcmp(command, "cd") == 0 ||
         strcmp(command, "pwd") == 0;
}

static void execute_command(char *cmd) {
  char *argv[MAX_ARGS + 1];

  char *token_storage = malloc(strlen(cmd) + 1);
  if (token_storage == NULL) {
    perror("malloc");
    return;
  }

  int argc = tokenize(cmd, argv, MAX_ARGS + 1, token_storage);

  /*fprintf(stderr, "argc = %d\n", argc);

  for (int i = 0; argv[i] != NULL; i++) {
    fprintf(stderr, "argv[%d] = <%s>\n", i, argv[i]);
  }*/

  if (argc < 0 || argc == 0) {
    free(token_storage);
    return;
  }

  if (is_builtin_command(argv[0])) {
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    if (saved_stdin == -1 || saved_stdout == -1) {
      perror("dup");
      if (saved_stdin != -1)
        close(saved_stdin);
      if (saved_stdout != -1)
        close(saved_stdout);
      free(token_storage);
      return;
    }

    if (apply_redirections(argv) == -1) {
      dup2(saved_stdin, STDIN_FILENO);
      dup2(saved_stdout, STDOUT_FILENO);
      close(saved_stdin);
      close(saved_stdout);
      free(token_storage);
      return;
    }

    run_builtin(argv);

    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);

    close(saved_stdin);
    close(saved_stdout);
    free(token_storage);
    return;
  }

  pid_t pid = fork();

  if (pid == -1) {
    perror("fork");
    free(token_storage);
    return;
  }

  if (pid == 0) {
    signal(SIGINT, SIG_DFL);

    if (apply_redirections(argv) == -1) {
      _exit(1);
    }

    if (argv[0] == NULL) {
      fprintf(stderr, "shell: missing command\n");
      _exit(1);
    }

    /*fprintf(stderr, "before execvp:\n");

    for (int i = 0; argv[i] != NULL; i++) {
      fprintf(stderr, "argv[%d] = <%s>\n", i, argv[i]);
    }*/

    execvp(argv[0], argv);
    perror(argv[0]);
    _exit(127);
  }

  int status = 0;

  while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {
  }

  free(token_storage);
}

static int execute_commands(char *line) {
  char *command_start = line;
  int in_single_quote = 0;
  int in_double_quote = 0;
  int escaped = 0;

  for (char *current = line; *current != '\0'; current++) {
    if (escaped) {
      escaped = 0;
      continue;
    }

    if (!in_single_quote && *current == '\\') {
      escaped = 1;
      continue;
    }

    if (!in_double_quote && *current == '\'') {
      in_single_quote = !in_single_quote;
      continue;
    }

    if (!in_single_quote && *current == '"') {
      in_double_quote = !in_double_quote;
      continue;
    }

    if (!in_single_quote && !in_double_quote && *current == ';') {
      *current = '\0';

      char *command = trim_whitespace(command_start);

      if (*command != '\0') {
        execute_command(command);
      }

      command_start = current + 1;
    }
  }

  if (in_single_quote || in_double_quote) {
    fprintf(stderr, "shell: unmatched quote\n");
    return -1;
  }

  char *command = trim_whitespace(command_start);

  if (*command != '\0') {
    execute_command(command);
  }

  return 0;
}

int main(void) {
  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len;
  struct sigaction signal_action = {0};

  signal_action.sa_handler = handle_sigint;
  sigemptyset(&signal_action.sa_mask);
  if (sigaction(SIGINT, &signal_action, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  while (1) {
    print_prompt();

    line_len = getline(&line, &line_cap, stdin);
    if (line_len == -1) {
      if (errno == EINTR && sigint_received) {
        sigint_received = 0;
        clearerr(stdin);
        putchar('\n');
        continue;
      }
      putchar('\n');
      free(line);
      return 0;
    }

    execute_commands(line);
  }
}
