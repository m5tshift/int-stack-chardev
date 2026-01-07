#ifndef INT_STACK_H
#define INT_STACK_H

#include <linux/ioctl.h>

#define IOCTL_MAGIC_NUM 'k'

#define IOCTL_SET_SIZE _IOW(IOCTL_MAGIC_NUM, 1, int)

#define IOCTL_GET_COUNT _IOR(IOCTL_MAGIC_NUM, 2, int)

#define DEVICE_NAME "int_stack"

#define USB_VID 0x058f
#define USB_PID 0x6387

#endif