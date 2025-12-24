#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/rwsem.h>
#include <linux/device.h>
#include "int_stack.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("m5tshift");
MODULE_DESCRIPTION("int_stack chardev");
MODULE_VERSION("1.0");

struct stack_dev {
    int *data;                // Массив данных
    int top;                  // Индекс верхушки (кол-во элементов)
    int capacity;
    struct rw_semaphore lock; // Блокировка reader-writer
    struct cdev cdev;         // Структура символьного устройства
    struct class *cl;         // Класс устройства для udev
    dev_t dev_no;             // Номер устройства
};

static struct stack_dev *my_stack_dev;
static int default_size = 0;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    int val;
    int ret_val;

    if (len < sizeof(int)) {
        return -EINVAL;
    }

    down_write(&my_stack_dev->lock);

    if (my_stack_dev->top == 0) {
        up_write(&my_stack_dev->lock);
        return 0;
    }

    my_stack_dev->top--;
    val = my_stack_dev->data[my_stack_dev->top];

    up_write(&my_stack_dev->lock);

    ret_val = copy_to_user(buffer, &val, sizeof(int));
    if (ret_val != 0) {
        return -EFAULT;
    }

    return sizeof(int);
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int val;
    int ret_val;

    if (len < sizeof(int)) {
        return -EINVAL;
    }

    ret_val = copy_from_user(&val, buffer, sizeof(int));
    if (ret_val != 0) {
        return -EFAULT;
    }

    down_write(&my_stack_dev->lock);

    if (my_stack_dev->top >= my_stack_dev->capacity) {
        up_write(&my_stack_dev->lock);
        return -ERANGE;
    }

    my_stack_dev->data[my_stack_dev->top] = val;
    my_stack_dev->top++;

    up_write(&my_stack_dev->lock);

    return sizeof(int);
}

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    int new_size;
    int *new_data;
    int ret = 0;

    switch (cmd) {
        case IOCTL_SET_SIZE:
            if (copy_from_user(&new_size, (int __user *)arg, sizeof(int))) {
                return -EFAULT;
            }
            if (new_size <= 0) {
                return -EINVAL;
            }

            down_write(&my_stack_dev->lock);
            
            new_data = krealloc(my_stack_dev->data, new_size * sizeof(int), GFP_KERNEL);
            if (!new_data) {
                up_write(&my_stack_dev->lock);
                return -ENOMEM;
            }

            my_stack_dev->data = new_data;
            my_stack_dev->capacity = new_size;
            
            if (my_stack_dev->top > new_size) {
                my_stack_dev->top = new_size;
            }
            
            up_write(&my_stack_dev->lock);
            break;

        case IOCTL_GET_COUNT:
            down_read(&my_stack_dev->lock);
            ret = my_stack_dev->top;
            up_read(&my_stack_dev->lock);
            
            if (copy_to_user((int __user *)arg, &ret, sizeof(int))) {
                return -EFAULT;
            }
            break;

        default:
            return -ENOTTY;
    }
    return 0;
}

static int __init stack_init(void) {
    int ret;
    dev_t dev;

    my_stack_dev = kzalloc(sizeof(struct stack_dev), GFP_KERNEL);
    if (!my_stack_dev) return -ENOMEM;

    init_rwsem(&my_stack_dev->lock);

    my_stack_dev->capacity = default_size;
    my_stack_dev->top = 0;
    my_stack_dev->data = kcalloc(default_size, sizeof(int), GFP_KERNEL);
    if (!my_stack_dev->data) {
        kfree(my_stack_dev);
        return -ENOMEM;
    }

    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0) goto err_data;

    my_stack_dev->dev_no = dev;

    my_stack_dev->cl = class_create("stack_class");
    if (IS_ERR(my_stack_dev->cl)) {
        ret = PTR_ERR(my_stack_dev->cl);
        goto err_unregister;
    }

    cdev_init(&my_stack_dev->cdev, &fops);

    ret = cdev_add(&my_stack_dev->cdev, my_stack_dev->dev_no, 1);
    if (ret < 0) goto err_class;

    if (device_create(my_stack_dev->cl, NULL, my_stack_dev->dev_no, NULL, DEVICE_NAME) == NULL) {
        ret = -1;
        goto err_cdev;
    }

    printk(KERN_INFO "int-stack: module loaded, capacity=%d\n", default_size);
    return 0;

err_cdev:
    cdev_del(&my_stack_dev->cdev);
err_class:
    class_destroy(my_stack_dev->cl);
err_unregister:
    unregister_chrdev_region(my_stack_dev->dev_no, 1);
err_data:
    kfree(my_stack_dev->data);
    kfree(my_stack_dev);
    return ret;
}

static void __exit stack_exit(void) {
    device_destroy(my_stack_dev->cl, my_stack_dev->dev_no);
    cdev_del(&my_stack_dev->cdev);
    class_destroy(my_stack_dev->cl);
    unregister_chrdev_region(my_stack_dev->dev_no, 1);
    
    kfree(my_stack_dev->data);
    kfree(my_stack_dev);
    
    printk(KERN_INFO "int_stack module unloaded\n");
}

module_init(stack_init);
module_exit(stack_exit);
