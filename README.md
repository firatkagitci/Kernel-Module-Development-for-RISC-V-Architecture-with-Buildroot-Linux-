# Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux
This repository is prepared to explain how to create the development environment in our case Buildroot-Linux running with QEMU emulating RISC-V architecture, and an implementation of SHA-256 encryption algorithm integrated into a kernel module. We developed a driver code on our Ubuntu host machine and used cross-compilation which is a toolchain you add to your host to translate the driver code you developed on your Ubuntu host to the target Buildroot-Linux as a kernel module. Buildroot is a simple, efficient tool for generating embedded Linux systems through cross-compilation. It's ideal for developing lightweight Linux versions for resource-constrained embedded devices. Keep in mind we do not create the kernel module on target Buildroot since it does not provide compilation tools due to its lightweight nature.

As a prerequisite, the person who reads this repository should be familiar with operating systems fundamentals and especially the basic theory of kernel modules.

This project is done on an Ubuntu host (version 23.10) and in our case running on a virtual machine created on the Virtual Box software, by Oracle. For instructions on setting this environment visit ubuntu.com and virtualbox.org. The commands here are compatible with Ubuntu/Debian distributions.

## Installing QEMU
The first step is installing Qemu from the git repository and beforehand make sure you have the necessary toolchains and dependencies:
Installing dependencies:
```console
sudo apt-get update
sudo apt-get install -y build-essential git ncurses-dev bison flex libssl-dev make gcc
sudo apt install ninja-build
sudo apt-get install pkg-config
sudo apt-get install libglib2.0-dev
sudo apt-get install libpixman-1-dev
```
**Warning:** In case you still encounter any dependency errors which is highly possible, read the error messages and install the required dependent tools indicated in the errors.
```console
git clone https://github.com/qemu/qemu.git
cd qemu
git checkout v6.0.0
```
Configure the system for risc-v architecture by the following command:
```console
./configure --target-list=riscv64-softmmu
make
```
## Installing Buildroot
Then you have to install Buildroot separately, not inside QEMU.
```console
git clone https://github.com/buildroot/buildroot.git
cd buildroot/
make qemu_riscv64_virt_defconfig
make
```

The last make command might take a long time, possibly more than one hour since it compiles the entire kernel of the target system.

After the kernel compilation command `make` you will see the image files inside `/buildroot/output/images` directory path. Go to the `/images` directory and you will see the script named start-qemu.sh. You need to modify the directory inside this script to you qemu/build directory path. 

buildroot/output/images/start_qemu.sh file content: 
```c
#!/bin/sh

BINARIES_DIR="${0%/*}/"
# shellcheck disable=SC2164
cd "${BINARIES_DIR}"

mode_serial=false
mode_sys_qemu=false
while [ "$1" ]; do
    case "$1" in
    --serial-only|serial-only) mode_serial=true; shift;;
    --use-system-qemu) mode_sys_qemu=true; shift;;
    --) shift; break;;
    *) echo "unknown option: $1" >&2; exit 1;;
    esac
done

if ${mode_serial}; then
    EXTRA_ARGS='-nographic'
else
    EXTRA_ARGS=''
fi

if ! ${mode_sys_qemu}; then
    export PATH="/home/asdf/Desktop/qemu/build:${PATH}"   # MODIFY THIS LINE TO QEMU/BUILD
fi

exec qemu-system-riscv64 -M virt -bios fw_jump.elf -kernel Image -append "rootwait root=/dev/vda ro" -drive file=rootfs.ext2,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -netdev user,id=net0 -device virtio-net-device,netdev=net0 -nographic  ${EXTRA_ARGS} "$@"
```
By adding ./ to the beginning: `./start-qemu.sh` , you can run the buildroot.

Keep in mind that every time you compile the buildroot, you need to change this file. But if you don't want to deal with this you can use the following command, remember to adjust the directory path to your system. 

```sh
exec /Desktop/qemu/build/qemu-system-riscv64 -M virt -bios fw_jump.elf -kernel Image -append "rootwait root=/dev/vda ro" -drive file=rootfs.ext2,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -netdev user,id=net0 -device virtio-net-device,netdev=net0 -nographic  ${EXTRA_ARGS} "$@"
```



This will boot the system with OpenSbi, and your Buildroot system will start. The password is 'root' and keep in mind that this system does not have a GUI(Graphical User Interface), meaning you will have to use the command line to interact with the new system we just created. The password: root.

![start](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/05354552-411d-481c-932a-9a34cb39c1a0)

---

![start1](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/fb920b34-cd7c-421a-8565-b8c51b3cc131)


## Creating Kernel Module For Ubuntu Host:
It is highly suggested that you start with a simple kernel module to test your system, in our case we will create a kernel module that gives a message as 'Hello World'. First, you test it on your host machine Ubuntu. Create a separate directory and name it as you wish (e.g. kmodules) and create the hello.c file `touch hello.c`.

Simple kernel module 'hello.c':
```c
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple hello world kernel module");

static int __init hello_init(void) {
    printk(KERN_INFO "Hello, world!\n");
    return 0; // Non-zero return means that the module couldn't be loaded.
}

static void __exit hello_cleanup(void) {
    printk(KERN_INFO "Cleaning up module.\n");
}

module_init(hello_init);
module_exit(hello_cleanup);
```
To compile the hello.c code we need to use a Makefile file that includes the following script: 

```make
obj-m += hello.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```
Here `uname -r` command gives the kernel header name of the system.

Keep in mind that just before `make` command (inside Makefile) there should be a tab-sized space. Save the Makefile and use `make` command to compile this C code to create a kernel module. After that, you will see multiple files created including hello.ko which is the kernel module created for the Ubuntu host. Now that we have the kernel module, we can add it to our main kernel by using the command `sudo insmod hello.ko`, now you need to run `dmesg` command to see its effect on the kernel messages prompt as "Hello, World!" message. 

 ## Cross-Compiling Kernel Module For Buildroot Linux running on RISC-V:

We have created a simple kernel module and executed it, this is a good practice to understand the kernel module development. We need to install cross-compilation tools to compile a kernel module for our target architecture RISC-V. The following command is used to install the required cross-compilation toolchain: 
 ```console
sudo apt-get update
sudo apt-get install gcc-riscv64-linux-gnu
```
 Create another directory for kernel modules of the Buildroot RISC-V based system. name it as you wish(e.g. kmodules_target)

 Inside, create the same hello.c, and create a new Makefile file. Inside the new Makefile write the following script, that uses cross-compilation tool:

```make
obj-m += hello.o
all:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C <path-to-buildroot-kernel> M=$(PWD) modules

clean:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C <path-to-buildroot-kernel> M=$(PWD) clean

 ```

**path-to-buildroot-kernel**: As an example, this path should be like the following: `/home/user/Desktop/buildroot/output/build/linux-6.6.36`  the user and the Linux version(on Buildroot) may be different on your project, so check it and modify accordingly.

Save the Makefile file, and use again the `make` command on the shell to compile. Then you will see multiple files created including hello.ko specifically crosscompiled for our new Buildroot-Linux development environment running on RISC-V architecture. You might question how this kernel module hello.ko is loaded to our development environment. Here are the steps:

Go to `/buildroot`, inside create a directory named as you wish (or name it as `overlay` for compatibility), inside create another path (e.g.  `/home/driver`)

Shortly, use this: `mkdir -p overlay/home/driver ``` The -p flag ensures that mkdir creates all necessary parent directories that do not exist.

Then copy the kernel module 'hello.ko' to `overlay/home/driver` directory.

This path is the path to your kernel header on your target environment.

After doing this, inside `/buildroot` directory you need to run `make menuconfig` which opens a text-based GUI. Keep in mind that this interface works with a dependency, so run the following command to resolve it if you receive an error: `sudo apt-get install libncurses5-dev libncursesw5-dev`


**Configure Buildroot to Use the Overlay:** You need to tell Buildroot to use this overlay directory when building the root filesystem image. This can be done in the Buildroot configuration:

After running `make menuconfig ` within your Buildroot directory;
Navigate to **System configuration > Root filesystem overlay directories.**
Write 'overlay' to the blank space.  
Save and exit the configuration menu.
![menuconfig1](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/4368e505-9d40-4866-a591-3cab16dcd5db)

![menuconfig](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/f4c2b4dd-74d0-4704-a61e-fdbfa6a2b23b)

Then run `make` inside `/buildroot`. Every time when you add a new file to the overlay directory, you need to recompile the buildroot so that you will see the changes in the buildroot.

To see if the kernel module is loaded to Buildroot, start the Buildroot by executing the command `./start-qemu.sh` inside `/buildroot/output/images/ `. then the Buildroot system will start, you need to check the directory `/home/driver`, there you should see the loaded kernel module. You can insert this kernel module (hello.ko) to the running kernel by using `insmod hello.ko` command, then if you use `dmesg` command you will see as an output the "Hello, World!" message. This means you have successfully cross-compiled a kernel module, added it to your target system, and then inserted it into the running kernel. 

![kernel_module_cross_compiled](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/e25b7ad9-6226-4dbb-8c17-88be071d1b51)

Until now we have practiced and learned:
- How to set the development environment Buildroot-Linux running on RISC-V architecture
- How to create a simple kernel module
- How to cross-compile a kernel module
- Adding the kernel module to the target system of Buildroot-Linux running on RISC-V architecture

These steps are also helpful in creating the cryptographic kernel module which is explained here as follows.

# SHA-256 Crypto Development
Check the file SHA-256 Crypto Core Developement.md Markdown file that include through explanation.
