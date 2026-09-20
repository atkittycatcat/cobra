#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 64

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

int main(void) {
  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len;

  while (1) {
    print_prompt();

    line_len = getline(&line, &line_cap, stdin);
    if (line_len == -1) {
      putchar('\n');
      free(line);
      return 0;
    }

    char *cmd = trim_whitespace(line);
    if (*cmd == '\0') {
      continue;
    }

    char *argv[MAX_ARGS + 1];
    int argc = 0;
    char *token = strtok(cmd, " \t\n\r");

    while (token != NULL && argc < MAX_ARGS) {
      argv[argc++] = token;
      token = strtok(NULL, " \t\n\r");
    }
    argv[argc] = NULL;

    if (argc == 0) {
      continue;
    }

    if (run_builtin(argv)) {
      continue;
    }

    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      continue;
    }

    if (pid == 0) {
      execvp(argv[0], argv);
      perror("execvp");
      _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);
  }
}
