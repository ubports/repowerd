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

static const char legacy_name[] = "legacy";
static const char state_path[] = "/sys/power/state";
static const char wakelock_path[] = "/sys/power/wake_lock";
static const char wakeup_count_path[] = "/sys/power/wakeup_count";
static const char mem_str[] = "mem";

#define WAKEUP_COUNT_LEN 64

static int wakeup_count_supported, wakeup_count_valid;
static char wakeup_count[WAKEUP_COUNT_LEN];

static int legacy_prepare(void)
{
    int ret;

    if (wakeup_count_supported) {
        ret = sysfs_read(wakeup_count_path, wakeup_count, WAKEUP_COUNT_LEN);
        if (ret < 0) {
            wakeup_count_valid = 0;
            return ret;
        }
        wakeup_count_valid = 1;
    }

    return 0;
}

static int legacy_enter(void)
{
    int ret;

    if (wakeup_count_supported && wakeup_count_valid) {
        wakeup_count_valid = 0;

        ret = sysfs_write(wakeup_count_path, wakeup_count,
                          strlen(wakeup_count));
        if (ret < 0) {
            /* Wakup happened since reading wakeup_count */
            return ret;
        }
    }

    ret = sysfs_write(state_path, mem_str, ARRAY_SIZE(mem_str) - 1);
    return ret < 0 ? ret : 0;
}

static const struct suspend_handler legacy_handler = {
    .name = legacy_name,
    .prepare = legacy_prepare,
    .enter = legacy_enter,
};

const struct suspend_handler *legacy_detect(void)
{
    if (sysfs_file_exists(state_path) && !sysfs_file_exists(wakelock_path)) {
        wakeup_count_supported = sysfs_file_exists(wakeup_count_path);
        return &legacy_handler;
    }

    return NULL;
}
