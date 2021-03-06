/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef INITNG_COMMON_H
#define INITNG_COMMON_H

#include <initng/misc.h>
#include <initng/active_db.h>

int initng_common_get_service(active_db_h * service);
int initng_common_mark_service(active_db_h * service, a_state_h * state);
int initng_common_service_blacklisted(const char *name);

void initng_common_state_lock(active_db_h * service);
int initng_common_state_unlock(active_db_h * service);
void initng_common_state_lock_all(void);
int initng_common_state_unlock_all(void);
a_state_h *initng_common_state_has_changed(active_db_h * service);

#endif /* INITNG_COMMON_H */
