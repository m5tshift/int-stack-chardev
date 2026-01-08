import os
import sys
import fcntl
import struct
import array
import subprocess


DEVICE_PATH = "/dev/int_stack"

IOCTL_SET_SIZE = 0x40046B01
IOCTL_GET_COUNT = 0x80046B02

USB_VID = 0x058F
USB_PID = 0x6387


def is_usb_plugged(vid, pid):
    device_id = f"{vid:04x}:{pid:04x}"
    result = subprocess.run(["lsusb", "-d", device_id], capture_output=True)
    return result.returncode == 0


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 kernel_stack.py <command> [args]")
        return

    cmd = sys.argv[1]

    if not is_usb_plugged(USB_VID, USB_PID):
        print(f"ERROR: USB key is not inserted (VID:{USB_VID:04x}, PID:{USB_PID:04x})")
        sys.exit(1)

    try:
        fd = os.open(DEVICE_PATH, os.O_RDWR)
    except OSError as e:
        print(f"ERROR: Could not open device ({e})")
        sys.exit(1)

    try:
        if cmd == "set-size":
            if len(sys.argv) < 3:
                return
            size = int(sys.argv[2])
            if size <= 0:
                print("ERROR: size should be > 0")
                return
            arg = array.array("i", [size])
            fcntl.ioctl(fd, IOCTL_SET_SIZE, arg)

        elif cmd == "push":
            if len(sys.argv) < 3:
                return
            val = int(sys.argv[2])
            try:
                os.write(fd, struct.pack("i", val))
            except OSError as e:
                if e.errno == 34:
                    print("ERROR: stack is full")
                    sys.exit(e.errno)
                raise

        elif cmd == "pop":
            data = os.read(fd, 4)
            if not data:
                print("NULL")
            else:
                print(struct.unpack("i", data)[0])

        elif cmd == "unwind":
            count_arg = array.array("i", [0])
            fcntl.ioctl(fd, IOCTL_GET_COUNT, count_arg)

            for _ in range(count_arg[0]):
                data = os.read(fd, 4)
                if data:
                    print(struct.unpack("i", data)[0])

        else:
            print(f"ERROR: Unknown command '{cmd}'")

    except Exception as e:
        print(f"ERROR: {e}")
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
