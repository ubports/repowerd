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

#ifndef SUSPENDIF_H
#define SUSPENDIF_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct suspend_handler {
    const char *name;
    int (*prepare)(void);
    int (*enter)(void);
    int (*exit)(void);
    int (*acquire_wake_lock)(const char *);
    int (*release_wake_lock)(const char *);
};

const struct suspend_handler *autosleep_detect(void);
const struct suspend_handler *earlysuspend_detect(void);
const struct suspend_handler *legacy_detect(void);
const struct suspend_handler *mocksuspend_detect(void);

#endif /* SUSPENDIF_H */
