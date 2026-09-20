#!/bin/bash
set -euo pipefail
giturl="https://github.com/atkittycatcat/cobra/tree/main"

tmpdir="$(mktemp -d)"

cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

cd $tmpdir

git clone $giturl

clear


systemwide() {
    sudo make install
}

useronly() {
    make PREFIX=$(HOME)/.local install
}

while true; do
    read -rp "User only install or system wide? [1 useronly, 2 systemwide]> " choice

    case "$choice" in
        1)
            systemwide
            break
            ;;
        2)
            useronly
            break
            ;;
        *)
            break
            ;;
    esac
done

cd ../
sudo rm -r cobra

setlogin() {
    sudo install -Dm755 build/cobra /usr/local/bin/cobra
    echo /usr/local/bin/cobra | sudo tee -a /etc/shells
    chsh -s /usr/local/bin/cobra
    cobra
}

while true; do
    read -rp "User only install or system wide? [1 useronly, 2 systemwide]> " choice

    case "$choice" in
        y)
            setlogin
            break
            ;;
        n)
            break
            ;;
    esac
done

