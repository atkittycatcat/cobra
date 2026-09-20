#!/usr/bin/env bash
set -euo pipefail

giturl="https://github.com/atkittycatcat/cobra.git"
tmpdir="$(mktemp -d)"
repo="$tmpdir/cobra"

cleanup() {
    sudo rm -rf -- "$tmpdir"
}
trap cleanup EXIT

if [[ ! -t 0 ]]; then
    if [[ -r /dev/tty ]]; then
        exec </dev/tty
    else
        printf 'This script requires an interactive terminal.\n' >&2
        exit 1
    fi
fi

git clone --depth 1 -- "$giturl" "$repo"
cd "$repo"

install_cobra() {
    while true; do
        read -rp "Install mode? [1 user-only, 2 system-wide] > " choice

        case "$choice" in
            1)
                make PREFIX="$HOME/.local" install
                cobra_path="$HOME/.local/bin/cobra"
                break
                ;;
            2)
                sudo make install
                cobra_path="/usr/local/bin/cobra"
                break
                ;;
            *)
                printf 'Invalid choice.\n'
                ;;
        esac
    done

    read -rp "Set Cobra as your login shell? [y/N] > " choice

    if [[ "${choice,,}" == "y" ]]; then
        if [[ "$cobra_path" != "/usr/local/bin/cobra" ]]; then
            printf 'Login-shell setup requires a system-wide installation.\n'
            return
        fi

        if ! grep -qxF "$cobra_path" /etc/shells; then
            printf '%s\n' "$cobra_path" | sudo tee -a /etc/shells >/dev/null
        fi

        chsh -s "$cobra_path"
        printf 'Login shell changed to Cobra.\n'
    fi
}

uninstall_cobra() {
    while true; do
        read -rp "Uninstall mode? [1 user-only, 2 system-wide] > " choice

        case "$choice" in
            1)
                make PREFIX="$HOME/.local" uninstall
                break
                ;;
            2)
                cobra_path="/usr/local/bin/cobra"
                current_shell="$(getent passwd "$USER" | cut -d: -f7)"

                if [[ "$current_shell" == "$cobra_path" ]]; then
                    printf 'Cobra is your current login shell.\n'
                    read -rp "Change it to Bash before uninstalling? [Y/n] > " answer

                    if [[ "${answer,,}" != "n" ]]; then
                        chsh -s "$(command -v bash)"
                    else
                        printf 'Uninstall cancelled.\n'
                        return
                    fi
                fi

                sudo make uninstall

                sudo sed -i '\|^/usr/local/bin/cobra$|d' /etc/shells
                break
                ;;
            *)
                printf 'Invalid choice.\n'
                ;;
        esac
    done
}

run_cobra() {
    make run
}

printf 'Cobra setup\n'
printf '1) Install\n'
printf '2) Uninstall\n'
printf '3) Run from source\n'
printf '4) Exit\n'

while true; do
    read -rp "> " action

    case "$action" in
        1)
            install_cobra
            break
            ;;
        2)
            uninstall_cobra
            break
            ;;
        3)
            run_cobra
            break
            ;;
        4)
            exit 0
            ;;
        *)
            printf 'Invalid choice.\n'
            ;;
    esac
done
