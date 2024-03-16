# Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux
This repository is prepared to explain how to create the development environment - in our case Buildroot-Linux running on QEMU emulating RISC-V architecture, and an implementation of SHA-256 cryptocore emulated on QEMU. We create a device driver on our Ubuntu host machine and use cross-compilation which is a toolchain we add to our host to translate the driver code we develop on our Ubuntu host to the target Buildroot-Linux as a kernel module. Buildroot is a simple, efficient tool for generating embedded Linux systems through cross-compilation. It's ideal for developing lightweight Linux versions for resource-constrained embedded devices. Keeping in mind that we do not create the kernel module on target Buildroot since it does not provide compilation tools due to its lightweight nature.

A prerequisity to delve in this project: some familiar with operating systems fundamentals. especially the basic theory of kernel modules.

This project has been developed on an Ubuntu host (version 23.10) running as a virtual machine on the software :Virtual Box by Oracle. For instructions on the installation of virtual box and ubuntu visit ubuntu.com and virtualbox.org. However, The commands are compatible with Ubuntu/Debian distributions.

## Installing QEMU
The first step is installing Qemu from the git repository and beforehand make sure you have the necessary toolchains and dependencies:
Installing dependencies:
```
sudo apt-get update
sudo apt-get install -y build-essential git ncurses-dev bison flex libssl-dev make gcc
sudo apt install ninja-build
sudo apt-get install pkg-config
sudo apt-get install libglib2.0-dev
sudo apt-get install libpixman-1-dev
```
**Warning:** In case you still encounter any dependency errors - which is highly possible - read the error messages and install the required dependent tools indicated in the errors.
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

After the kernel compilation command 'make' you will see the image files inside `/buildroot/output` directory path. Go to the `/images` directory and execute the file named start-qemu.sh by adding ./ to the beginning: `./start-qemu.sh`

This will boot the system with OpenSbi, and your Buildroot system will start. have in mind that this system does not have a GUI (Graphical User Interface), meaning you will have to use the command line (like a real developer) to interact with the new system you just created. The password is: root.

![start](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/05354552-411d-481c-932a-9a34cb39c1a0)

---

![start1](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/fb920b34-cd7c-421a-8565-b8c51b3cc131)


## Creating Kernel Module For Ubuntu Host:
It is highly suggested that you start with a simple kernel module to test your system, The most commona kernel module to do this is the simple "hello world" device driver, which prints the message 'Hello World' in the kernel when it's loaded, and prints a goodbye message when it is removed from the kernel.

First, you test it on your host machine Ubuntu. for that; create a separate directory and name it as you wish (e.g. kmodules) and create the hello.c device driver ('touch hello.c').

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
To compile the code we need to use the following Makefile file: 

```
obj-m += hello.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```
Here `uname -r` command gives the kernel header name of the system.

Keep in mind that just before 'make' command inside this script there should be a 'tab' sized space. Save the Makefile and use `make` command to compile this code to create a kernel module. This will result in creating multiple files including hello.ko which is the kernel module created for the Ubuntu host.

Now that we have the kernel module, we can add it to our main kernel by using the command `sudo insmod hello.ko`, now you need to run `dmesg` command to see its effect on the kernel log; "Hello, World!" message. and the command `sudo rmmod hello.ko` removes it, the correct execution of the command can be confiremed by finding the message "Cleaning up module." in the kernel log, again, with `dmsg` command.

 ## Cross-Compiling Kernel Module For Buildroot Linux running on RISC-V:

We have created a simple kernel module and executed it. This is a good exercise to understand the kernel module development
Now, We will install cross-compilation tools to compile a kernel module for our target architecture RISC-V. The following command is used to install the required cross-compilation toolchain: 
 ```
sudo apt-get update
sudo apt-get install gcc-riscv64-linux-gnu
```
 Create another directory for kernel modules Buildroot RISC-V based system. name it as you wish(e.g. kmodules_target)

 Inside, place the same hello.c driver, and create a new Makefile file with following script:

```
obj-m += hello.o
all:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C <path-to-buildroot-kernel> M=$(PWD) modules

clean:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C <path-to-buildroot-kernel> M=$(PWD) clean

 ```

**path-to-buildroot-kernel**: this path should be like the following: `/home/user/Desktop/qemu/buildroot/output/build/linux-6.1.44`  the 'user' and the Linux version(on Buildroot) may be different on your project, so check it and modify accordingly.

Save the Makefile file, and use again the `make` command so that you compile. again, multiple files will be created including hello.ko that has specifically been crosscompiled for our new Buildroot-Linux development environment running on RISC-V architecture.

You might question how this kernel module hello.ko is loaded to our development environment. Here are the steps:

Go to `/qemu/buildroot`, inside it create a directory named as you wish (or name it `overlay` for compatibility), inside create another path that is the same as in the target development environment environment (e.g.  `/lib/modules/6.1.44/extra`)

`mkdir -p overlay/lib/modules/6.1.44/extra/ ` The -p flag ensures that mkdir creates all necessary parent directories that do not exist.

Then copy the kernel module 'hello.ko' to `overlay/lib/modules/6.1.44/extra` directory.

This path is the path to your kernel header on your target environment. The version of the kernel header may be different on your project, so, again check accordingly. 

After doing this, inside `/qemu/buildroot` directory you need to run `make menuconfig` which opens a text-based GUI. However, this interface requires installing some dependencies. so, run the following command to resolve it if you receive an error: `sudo apt-get install libncurses5-dev libncursesw5-dev`


**Configure Buildroot to Use the Overlay:** You need to configure Buildroot to use overlay directory when building the root filesystem image. This can be done in Buildroot configuration menu:

Run `make menuconfig ` within your Buildroot directory and the configuiration menu will pop up.
Navigate to **System configuration > Root filesystem overlay directories.**
Add the path to your `overlay` directory. If you named your directory overlay and it's located in the root of your Buildroot directory, the path would simply be /overlay.
Save and exit the configuration menu.
![menuconfig1](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/4368e505-9d40-4866-a591-3cab16dcd5db)

![menuconfig](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/f4c2b4dd-74d0-4704-a61e-fdbfa6a2b23b)

Then run `make` inside `/buildroot` 

To see if the kernel module is loaded to Buildroot, start the Buildroot by executing the command `./start-qemu.sh` inside `/buildroot/output/images/ `. then the Buildroot system will start, you need to check the directory `/lib/modules/6.1.44/extra/`, there you should see the loaded kernel module. You can insert this kernel module to the running kernel by using `insmod hello.ko` command, then if you use `dmesg` command you will see as an output the "Hello, World!" message in the kernel log. This means you have successfully cross-compiled a kernel module, added it to your target system, and then inserted it into the running kernel. 

![kernel_module_cross_compiled](https://github.com/firatkagitci/Kernel-Module-Development-for-RISC-V-Architecture-with-Buildroot-Linux-/assets/72497084/e25b7ad9-6226-4dbb-8c17-88be071d1b51)

Until now we have practiced and learned:
- How to create a kernel module
- How to set up the development environment Buildroot-Linux running on RISC-V architecture
- How to cross-compile a kernel module
- Adding the kernel module to the target system of Buildroot-Linux running on RISC-V architecture

These steps are also helpful in creating the cryptographic kernel module which is explained in the following section.

# SHA-256 Cryptocore Development
to be continued...
