#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>

#define DEVICE_NAME "sha256"
#define CLASS_NAME "sha"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("A Linux kernel module for a SHA256 hardware accelerator");
MODULE_VERSION("0.1");

#define SHA256_BASE_ADDR 0x4000000
#define CTRL_REG 0x0008
#define STATUS_REG 0x000C
#define OUTPUT_REG 0x0410
#define INPUT_REG 0x0010
#define ENABLE 0x00000001

#define IOCTL_SHA256 _IOR('S', 0, uint32_t[8]) // Define IOCTL command for SHA256 hashing

static int majorNumber;
static struct class *shaClass = NULL;
static struct device *shaDevice = NULL;

static int sha_open(struct inode *inodep, struct file *filep);
static int sha_release(struct inode *inodep, struct file *filep);
static long sha_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t sha_read(struct file *filep, char *buffer, size_t len, loff_t *offset);
static int __init sha_init(void);
static void __exit sha_exit(void);

static struct file_operations fops = {
    .open = sha_open,
    .release = sha_release,
    .unlocked_ioctl = sha_ioctl,
    .read = sha_read,
};

static int sha_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "SHA256: Device has been opened\n");
    return 0;
}

static int sha_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "SHA256: Device successfully closed\n");
    return 0;
}

static long sha_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    char user_data[1024]; // Buffer for user data
    volatile void __iomem *input_addr;
    volatile void __iomem *output_addr;
    uint32_t output_data[8];

    switch (cmd) {
        case IOCTL_SHA256:
            if (copy_from_user(user_data, (char __user *)arg, sizeof(user_data))) {
                printk(KERN_WARNING "Failed to copy data from user\n");
                return -EFAULT;
            }

            input_addr = ioremap(SHA256_BASE_ADDR + INPUT_REG, sizeof(user_data));
            if (!input_addr) {
                printk(KERN_ERR "Failed to map input register\n");
                return -EIO;
            }
            memcpy_toio(input_addr, user_data, sizeof(user_data));
            iounmap(input_addr);

            // Enable the SHA256 device
            writel(ENABLE, ioremap(SHA256_BASE_ADDR + CTRL_REG, sizeof(uint32_t)));

            // Delay for simulation of processing (remove or adjust in actual hardware usage)
            mdelay(100);

          /*
            // Read the output
            output_addr = ioremap(SHA256_BASE_ADDR + OUTPUT_REG, sizeof(output_data));
            memcpy_fromio(output_data, output_addr, sizeof(output_data));
            iounmap(output_addr);

            if (copy_to_user((uint32_t __user *)arg, output_data, sizeof(output_data))) {
                printk(KERN_WARNING "Failed to copy data to user\n");
                return -EFAULT;
            }
          */
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

static ssize_t sha_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    volatile void __iomem *output_addr;
    uint32_t output_data[8];

    output_addr = ioremap(SHA256_BASE_ADDR + OUTPUT_REG, sizeof(output_data));
    if (!output_addr) {
        printk(KERN_ERR "Failed to map output register\n");
        return -EIO;
    }

    memcpy_fromio(output_data, output_addr, sizeof(output_data));
    iounmap(output_addr);

    if (copy_to_user(buffer, output_data, sizeof(output_data))) {
        printk(KERN_WARNING "Failed to copy data to user\n");
        return -EFAULT;
    }

    return sizeof(output_data);
}

static int __init sha_init(void) {
    printk(KERN_INFO "SHA256: Initializing the SHA256 LKM\n");
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "SHA256 failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "SHA256: registered correctly with major number %d\n", majorNumber);

    shaClass = class_create(CLASS_NAME);
    if (IS_ERR(shaClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(shaClass);
    }
    printk(KERN_INFO "SHA256: device class registered correctly\n");

    shaDevice = device_create(shaClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(shaDevice)) {
        class_destroy(shaClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(shaDevice);
    }
    printk(KERN_INFO "SHA256: device class created correctly\n");
    return 0;
}

static void __exit sha_exit(void) {
    device_destroy(shaClass, MKDEV(majorNumber, 0));
    class_unregister(shaClass);
    class_destroy(shaClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "SHA256: Goodbye from the LKM!\n");
}

module_init(sha_init);
module_exit(sha_exit);

