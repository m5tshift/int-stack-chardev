import os
import sys
import fcntl
import struct
import array
import errno

DEVICE_PATH = "/dev/int_stack"

IOCTL_SET_SIZE = 0x40046B01
IOCTL_GET_COUNT = 0x80046B02


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 kernel_stack.py <command> [args]")
        return

    cmd = sys.argv[1]

    try:
        fd = os.open(DEVICE_PATH, os.O_RDWR)
    except OSError as e:
        if e.errno == errno.ENOENT:
            print("ERROR: USB key is not inserted")
            sys.exit(1)
        else:
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
