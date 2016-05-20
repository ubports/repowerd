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
#include <errno.h>
#include "libsuspend.h"
#include "common.h"

const struct suspend_handler *handler;

void libsuspend_init(int force_mock)
{
    if (!force_mock) {
        handler = earlysuspend_detect();
        if (handler)
            return;

        handler = autosleep_detect();
        if (handler)
            return;

        handler = legacy_detect();
        if (handler)
            return;

        printf("No suspend interface detected, using mock suspend\n");
    }

    handler = mocksuspend_detect();
}

const char *libsuspend_getname(void)
{
    if (!handler)
        return "not-initialized yet";

    return handler->name;
}

int libsuspend_prepare_suspend(void)
{
    if (!handler)
        return -ENODEV;

    if (handler->prepare)
        return handler->prepare();

    return 0;
}

int libsuspend_enter_suspend(void)
{
    if (!handler)
        return -ENODEV;

    if (handler->enter)
        return handler->enter();

    return 0;
}

int libsuspend_exit_suspend(void)
{
    if (!handler)
        return -ENODEV;

    if (handler->exit)
        return handler->exit();

    return 0;
}

int libsuspend_acquire_wake_lock(const char *name)
{
    if (!handler)
        return -ENODEV;

    if (handler->acquire_wake_lock)
        return handler->acquire_wake_lock(name);

    return 0;
}

int libsuspend_release_wake_lock(const char *name)
{
    if (!handler)
        return -ENODEV;

    if (handler->release_wake_lock)
        return handler->release_wake_lock(name);

    return 0;
}
