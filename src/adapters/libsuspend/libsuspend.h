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

#ifndef LIBSUSPEND_H
#define LIBSUSPEND_H

#ifdef __cplusplus
extern "C" {
#endif

void libsuspend_init(int force_mock);
const char *libsuspend_getname(void);
int libsuspend_prepare_suspend(void);
int libsuspend_enter_suspend(void);
int libsuspend_exit_suspend(void);
int libsuspend_acquire_wake_lock(const char *name);
int libsuspend_release_wake_lock(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* LIBSUSPEND_H */
