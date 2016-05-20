/*
 * Copyright 2013 Canonical Ltd.
 *
 * This file is part of powerd.
 *
 * powerd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * powerd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int sysfs_file_exists(const char *path)
{
    return !access(path, F_OK);
}

int sysfs_read(const char *path, void *buf, int len)
{
    int fd;
    ssize_t ret;

    fd = open(path, O_RDONLY);
    if (fd == -1)
        return -errno;

    ret = read(fd, buf, len);
    if (ret == -1)
        ret = -errno;

    close(fd);
    return ret;
}

int sysfs_write(const char *path, const void *buf, int len)
{
    int fd;
    ssize_t ret;

    fd = open(path, O_WRONLY);
    if (fd == -1)
        return -errno;

    ret = write(fd, buf, len);
    if (ret == -1)
        ret = -errno;

    close(fd);
    return ret;
}
