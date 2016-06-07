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
#include "common.h"

static const char mocksuspend_name[] = "mocksuspend";

static int mocksuspend_prepare(void)
{
    return 0;
}

static int mocksuspend_enter(void)
{
    return 0;
}

static int mocksuspend_exit(void)
{
    return 0;
}

static int mocksuspend_acquire_wake_lock(const char *name)
{
    (void)name;
    return 0;
}

static int mocksuspend_release_wake_lock(const char *name)
{
    (void)name;
    return 0;
}

static const struct suspend_handler mocksuspend_handler = {
    .name = mocksuspend_name,
    .prepare = mocksuspend_prepare,
    .enter = mocksuspend_enter,
    .exit = mocksuspend_exit,
    .acquire_wake_lock = mocksuspend_acquire_wake_lock,
    .release_wake_lock = mocksuspend_release_wake_lock,
};

const struct suspend_handler *mocksuspend_detect(void)
{
    return &mocksuspend_handler;
}
