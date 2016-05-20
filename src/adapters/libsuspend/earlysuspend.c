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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include "common.h"
#include "sysfs.h"

/* Use to suppress unused parameter warnings */
#define __unused __attribute__ ((unused))

enum fb_state {
    FB_SLEEP,
    FB_AWAKE,

    NUM_FB_STATES
};
static enum fb_state fb_state = FB_AWAKE;

static const char *fb_file_names[] = {
    [FB_SLEEP] = "/sys/power/wait_for_fb_sleep",
    [FB_AWAKE] = "/sys/power/wait_for_fb_wake"
};

static pthread_t fb_monitor_thread;
static pthread_mutex_t fb_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fb_state_cond = PTHREAD_COND_INITIALIZER;
static int wait_for_fb = 0;

static const char earlysuspend_name[] = "earlysuspend";
static const char state_path[] = "/sys/power/state";
static const char wakelock_path[] = "/sys/power/wake_lock";
static const char wakeunlock_path[] = "/sys/power/wake_unlock";

static const char mem_str[] = "mem";
static const char on_str[] = "on";

static int wait_for_file(const char *fname)
{
    int fd, ret;
    char buf;

    fd = open(fname, O_RDONLY);
    if (fd == -1)
        return -errno;

    do {
        ret = read(fd, &buf, 1);
    } while (ret == -1 && errno == EINTR);

    close(fd);
    return ret == -1 ? -errno : 0;
}

static void *fb_monitor_thread_func(__unused void *unused)
{
    enum fb_state next_state;
    int ret;

    while (1) {
        next_state = fb_state == FB_SLEEP ? FB_AWAKE : FB_SLEEP;
        ret = wait_for_file(fb_file_names[next_state]);
        if (ret)
            continue;

        pthread_mutex_lock(&fb_state_mutex);
        fb_state = next_state;
        pthread_cond_signal(&fb_state_cond);
        pthread_mutex_unlock(&fb_state_mutex);
    }

    wait_for_fb = 0;
    return NULL;
}

/* Returns 1 if fb monitor thread started, 0 otherwise */
static int start_fb_monitor_thread(void)
{
    if (access(fb_file_names[FB_SLEEP], F_OK))
        return 0;
    if (access(fb_file_names[FB_AWAKE], F_OK))
        return 0;

    return !pthread_create(&fb_monitor_thread, NULL,
                           fb_monitor_thread_func, NULL);
}

static int earlysuspend_enter(void)
{
    int ret;
    int len = ARRAY_SIZE(mem_str) - 1;
   
    ret = sysfs_write(state_path, mem_str, len);
    if (ret == len && wait_for_fb) {
        pthread_mutex_lock(&fb_state_mutex);
        while (fb_state != FB_SLEEP)
            pthread_cond_wait(&fb_state_cond, &fb_state_mutex);
        pthread_mutex_unlock(&fb_state_mutex);
    }
    return ret < 0 ? ret : 0;
}

static int earlysuspend_exit(void)
{
    int ret;
    int len = ARRAY_SIZE(on_str) - 1;
   
    ret = sysfs_write(state_path, on_str, len);
    if (ret == len && wait_for_fb) {
        pthread_mutex_lock(&fb_state_mutex);
        while (fb_state != FB_AWAKE)
            pthread_cond_wait(&fb_state_cond, &fb_state_mutex);
        pthread_mutex_unlock(&fb_state_mutex);
    }
    return ret < 0 ? ret : 0;
}

static int earlysuspend_acquire_wake_lock(const char *name)
{
    int ret = sysfs_write(wakelock_path, name, strlen(name));
    return ret < 0 ? ret : 0;
}

static int earlysuspend_release_wake_lock(const char *name)
{
    int ret = sysfs_write(wakeunlock_path, name, strlen(name));
    return ret < 0 ? ret : 0;
}

static const struct suspend_handler earlysuspend_handler = {
    .name = earlysuspend_name,
    .enter = earlysuspend_enter,
    .exit = earlysuspend_exit,
    .acquire_wake_lock = earlysuspend_acquire_wake_lock,
    .release_wake_lock = earlysuspend_release_wake_lock,
};

const struct suspend_handler *earlysuspend_detect(void)
{
    int len, ret;
    if (sysfs_file_exists(wakelock_path) &&
        sysfs_file_exists(state_path)) {

        len = ARRAY_SIZE(on_str) - 1;
        ret = sysfs_write(state_path, on_str, len);
        if (ret != len) {
            return NULL;
        }

        wait_for_fb = start_fb_monitor_thread();
        return &earlysuspend_handler;
    }

    return NULL;
}
