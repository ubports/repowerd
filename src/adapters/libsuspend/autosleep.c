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

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "sysfs.h"

static const char autosleep_name[] = "autosleep";
static const char autosleep_path[] = "/sys/power/autosleep";
static const char wakelock_path[] = "/sys/power/wake_lock";
static const char wakeunlock_path[] = "/sys/power/wake_unlock";
static const char mem_str[] = "mem";
static const char off_str[] = "off";

static int autosleep_enter(void)
{
    int ret = sysfs_write(autosleep_path, mem_str, ARRAY_SIZE(mem_str) - 1);
    return ret < 0 ? ret : 0;
}

static int autosleep_exit(void)
{
    int ret = sysfs_write(autosleep_path, off_str, ARRAY_SIZE(off_str) - 1);
    return ret < 0 ? ret : 0;
}

static int autosleep_acquire_wake_lock(const char *name)
{
    int ret = sysfs_write(wakelock_path, name, strlen(name));
    return ret < 0 ? ret : 0;
}

static int autosleep_release_wake_lock(const char *name)
{
    int ret = sysfs_write(wakeunlock_path, name, strlen(name));
    return ret < 0 ? ret : 0;
}

static const struct suspend_handler autosleep_handler = {
    .name = autosleep_name,
    .enter = autosleep_enter,
    .exit = autosleep_exit,
    .acquire_wake_lock = autosleep_acquire_wake_lock,
    .release_wake_lock = autosleep_release_wake_lock,
};

const struct suspend_handler *autosleep_detect(void)
{
    if (!sysfs_file_exists(autosleep_path))
        return NULL;
    return &autosleep_handler;
}
