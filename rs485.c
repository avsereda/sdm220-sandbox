#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

#include "rs485.h"

#define BAUD_RATE B9600

static int tty_dev = -1;

static inline int raw_tty_open(const char *path, speed_t baud_rate)
{
    int fd = -1;
    struct termios term_iface = {0, };

    fd = open(path, O_RDWR|O_NOCTTY|O_NONBLOCK);
    if (fd < 0 || !isatty(fd))
        return -1;

    if (tcgetattr(fd, &term_iface) < 0)
        return -1;

    cfsetispeed(&term_iface, baud_rate);
    cfsetospeed(&term_iface, baud_rate);
    cfmakeraw(&term_iface);

    term_iface.c_cc[VMIN]  = 1;
    term_iface.c_cc[VTIME] = 0;
    term_iface.c_cflag &= ~(CSTOPB);
    term_iface.c_cflag |= (CSIZE|CS8|PARODD|CRTSCTS);

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &term_iface) < 0)
    	return -1;

    return fd;
}

static inline bool file_exist_and_character_device(const char *path)
{
    struct stat stat_buf = {0, };

    if (path == NULL || path[0] == '\0')
        return false;

    if (stat(path, &stat_buf) != 0)
        return false;

    return S_ISCHR(stat_buf.st_mode);
}

void rs485_init(const char *path)
{
    if (!file_exist_and_character_device(path)) {
        fprintf(stderr, "Bad device path: %s.\n", path);
        exit(EXIT_FAILURE);
    }

    tty_dev = raw_tty_open(path, BAUD_RATE);

    if (tty_dev < 0) {
        perror("raw_tty_open");
        exit(EXIT_FAILURE);
    }
}

bool rs485_available(void)
{
    int ret = 0;
    struct pollfd fds = {0, };

    fds.fd = tty_dev;
    fds.events = POLLIN;

    ret = poll(&fds, 1, 0);
    if (ret < 0) {
        perror("poll");
        exit(EXIT_FAILURE);
    }

    return ret == 1;
}

bool rs485_read_byte_nonblocking(uint8_t *result)
{
    int ret = 0;

    if (!rs485_available())
        return false;

    ret = read(tty_dev, result, 1);
    if (ret < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    return ret == 1;
}

bool rs485_read_byte(uint8_t *result)
{
    while (!rs485_available()) {}

    return rs485_read_byte_nonblocking(result);
}

void rs485_write_byte(uint8_t byte)
{
    int ret = 0;

    ret = write(tty_dev, &byte, 1);
    if (ret < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void rs485_write(uint8_t *buf, size_t buf_size)
{
    size_t i = 0u;

    for (; i < buf_size; ++i)
        rs485_write_byte(buf[i]);
}

