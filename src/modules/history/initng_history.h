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

#ifndef INITNG_HISTORY_H
#define INITNG_HISTORY_H
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <initng.h>

#define HISTORY 800

typedef struct history_s history_h;
struct history_s {
	active_db_h *service;
	char *name;
	double duration;		/* The time in seconds the service
					 * stayed in this state */
	struct timeval time;
	char *data;
	a_state_h *action;

	list_t list;
};


extern history_h history_db;

#define while_history_db(current) \
	initng_list_foreach(current, &history_db.list, list)

#define while_history_db_prev(current) \
	initng_list_foreach_rev(current, &history_db.list, list)
	
#define while_history_db_safe(current, safe) \
	initng_list_foreach_safe(current, safe, &history_db.list, list)
	
#define while_history_db_safe_prev(current, safe) \
	initng_list_foreach_rev_safe(current, safe, &history_db.list, list)

#endif /* ! INITNG_HISTORY_H */
