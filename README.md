# Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux
This repository is prepared to explain how to create the development environment in our case Buildroot-Linux running on QEMU emulating RISC-V architecture. We create a kernel module on our Ubuntu host machine and use cross-compilation which is a toolchain you add to your host to translate the kernel module you develop on your Ubuntu host to the target Buildroot-Linux. Buildroot is a simple, efficient tool for generating embedded Linux systems through cross-compilation. It's ideal for developing lightweight Linux versions for resource-constrained embedded devices. Keep in mind we do not create the kernel module on target Buildroot since it does not provide compilation tools due to its lightweight nature.

This project is done on an Ubuntu host (version 23.10) and in our case running on a virtual machine created on the Virtual Box software, by Oracle. For instructions on setting this environment visit ubuntu.com and virtualbox.org. The commands here are compatible with Ubuntu/Debian distributions.

## Installing QEMU
The first step is installing Qemu from the git repository and beforehand make sure you have the necessary toolchains and dependencies:
Installing dependencies:
```
sudo apt-get update
sudo apt-get install -y build-essential git ncurses-dev bison flex libssl-dev make gcc
```
**Warning:** In case you still encounter any dependency errors which is highly possible, read the error messages and install the required dependent tools indicated in the errors.
```
git clone https://github.com/qemu/qemu.git
cd qemu
git checkout v6.0.0
```
Configure the system for risc-v architecture by the following command:
```
./configure --target-list=riscv64-softmmu
make
```
## Installing Buildroot
Then you have to install Buildroot to the QEMU, you should be inside /qemu directory. 
```
git clone https://github.com/buildroot/buildroot.git
cd buildroot/
make qemu_riscv64_virt_defconfig
make
```

The last make command might take a long time, possibly more than 30 minutes since it compiles the entire kernel of the target system.

After the kernel compilation command 'make' you will see the image files inside /buildroot/output directory path. Go to the /images directory and execute the file named start-qemu.sh by adding ./ to the beginning: `./start-qemu.sh`

This will boot the system with OpenSbi, and your Buildroot system will start. The password is 'root' and keep in mind that this system does not have a GUI(Graphical User Interface), meaning you will have to use the command line to interact with the new system we just created.

## Creating Kernel Module For Ubuntu Host:
It is highly suggested that you start with a simple kernel module to test your system, in our case we will create a kernel module that gives a message as 'Hello World'. First, you test it on your host machine Ubuntu. Create a separate directory and name it as you wish (e.g. kmodules) and create the hello.c file ('touch hello.c').

Simple kernel module 'hello.c':
```
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

```
obj-m += hello.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

Keep in mind that just before 'make' command inside this script there should be a 'tab' sized space. Save the Makefile and use 'make' command to compile this C code to create a kernel module. After that, you will see multiple files created including hello.ko which is the kernel module created for the Ubuntu host. Now that we have the kernel module, we can add it to our main kernel by using the command 'sudo insmod hello.ko', now you need to run 'dmesg' command to see its effect on the kernel messages prompt as "Hello, World!" message. 

 ## Cross-Compiling Kernel Module For Buildroot Linux running on RISC-V:

 We have created a simple kernel module and executed it and this is a good practice to understand the kernel module development. We need to install cross-compilation tools to compile a kernel module for our target architecture RISC-V. The following command is used to install the required cross-compilation toolchain: 
 ```
sudo apt-get update
sudo apt-get install gcc-riscv64-linux-gnu
```
 Create another directory for kernel modules Buildroot RISC-V based system. name it as you wish(e.g. kmodules_target)

 Inside, create the same hello.c, and create a new Makefile file. Inside the new Makefile write the following script:

```
obj-m += hello.o
all:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C <path-to-buildroot-kernel> M=$(PWD) modules

clean:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C <path-to-buildroot-kernel> M=$(PWD) clean

 ```

<path-to-buildroot-kernel> : as an example, this path should be as following: '/home/user/Desktop/qemu/buildroot/output/build/linux-6.1.44 '  the 'user' and the Linux version may be different on your project, so check it accordingly. This is the Linux version of Buildroot. 

Save the Makefile file, and use again the 'make' command so that you compile. Then you will see multiple files created including hello.ko specifically crosscompiled for our new Buildroot-Linux development environment running on RISC-V architecture. You might question how this kernel module hello.ko is loaded to our development environment. Here are the steps:

Go to /qemu/buildroot, inside create a directory named as you wish (or name it 'overlay' for compatibility), inside create another path that is the same as in the target development environment environment (e.g.  /lib/modules/6.1.44/extra)

'mkdir -p overlay/lib/modules/6.1.44/extra/' The -p flag ensures that mkdir creates all necessary parent directories that do not exist.

This path is the path to your kernel header on your target environment. The version of the kernel header may be different on your project, so, again check accordingly. 

After doing this, inside /qemu/buildroot directory you need to run 'make menucofig' which opens a text-based GUI. Keep in mind that this interface works with a dependency, so run the following command to resolve it if you receive an error: `sudo apt-get install libncurses5-dev libncursesw5-dev`


Configure Buildroot to Use the Overlay: You need to tell Buildroot to use this overlay directory when building the root filesystem image. This can be done in the Buildroot configuration:

Run 'make menuconfig' within your Buildroot directory.
Navigate to System configuration > Root filesystem overlay directories.
Add the path to your 'overlay' directory. If you named your directory overlay and it's located in the root of your Buildroot directory, the path would simply be overlay.
Save and exit the configuration menu.

Then run 'make'
