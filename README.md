# Kernele-Module-Developement-for-RISC-V-Architecture-with-Buildroot-Linux
This repo is prepared to explain how to create the development environment as Buildroot running on QEMU emulating RISC-V architecture. We basically create a kernel on our Ubuntu host machine and use cross-compilation which is a toolchain you add to your host to translate the kernel module you develop on your Ubuntu host to the target Buildroot. 

This project is done on an Ubuntu host (version 23.10), which is a virtual machine created on the Virtual Box software, by Oracle. For instructions on setting this environment visit ubuntu.com and virtualbox.org. The commands here are compatible with Ubuntu/Debian-based platforms, that's why make sure you also use the same system. 

The first step is installing Qemu from the git repository and boeforehand make sure you have necessary toolchanics and dependencies:
Installing dependencies:
`sudo apt-get update
sudo apt-get install -y build-essential git ncurses-dev bison flex libssl-dev make gcc
`
Installing QEMU:
`git clone https://github.com/qemu/qemu.git`
`cd qemu`
`git checkout v6.0.0`

