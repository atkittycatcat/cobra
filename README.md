# Cobra

Cobra is a free and open source shell written in C, assisted (not fully written) by AI.

## Install script
```shell
curl -fsSL https://raw.githubusercontent.com/atkittycatcat/cobra/refs/heads/main/setup.sh | sh
```

## Install manually
To install the shell, go clone this code and cd into it:
```shell
git clone https://github.com/atkittycatcat/cobra.git && cd cobra
```
then install it:
```shell
make PREFIX=$(HOME)/.local install
```
or system wide:
```shell
sudo make install
```
and then cd out and remove the directory
```shell
cd ../ && rm -r cobra
```
Or just mash it up into one command
system wide:
```shell
git clone https://github.com/atkittycatcat/cobra.git && cd cobra && sudo make install && cd ../ && sudo rm -r cobra
```
user only:
```shell
git clone https://github.com/atkittycatcat/cobra.git && cd cobra && make PREFIX=$(HOME)/.local install && cd ../ && sudo rm -r cobra
```
and then 
```shell
cobra
```
## Uninstall
system wide:
```shell
git clone https://github.com/atkittycatcat/cobra.git && cd cobra && sudo make uninstall && cd ../ && rm -r cobra
```
user only:
```shell
git clone https://github.com/atkittycatcat/cobra.git && cd cobra && make PREFIX=$(HOME)/.local uninstall && cd ../ && rm -r cobra
```

## Run 
```shell
git clone https://github.com/atkittycatcat/cobra.git && cd cobra && make run
```
