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

#include <initng.h>

#include <stdio.h>
#include <stdlib.h>		/* free() exit() */
#include <string.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};


s_entry LAST = {
	.name = "last",
	.description = "If this option is set, you will be sure this service "
	    "is started last.",
	.type = SET,
	.ot = NULL,
};

stype_h *TYPE_PROVIDED;

/* returns TRUE if all use deps are started */
static void check_last(s_event * event)
{
	active_db_h *service;
	active_db_h *current = NULL;

	assert(event->event_type == &EVENT_START_DEP_MET);
	assert(event->data);

	service = event->data;

	assert(service->name);

	/* And LAST is set. */
	if (!is(&LAST, service))
		return;

	if (!TYPE_PROVIDED)
		TYPE_PROVIDED = initng_service_type_get_by_name("provided");

	/* ok check with all service */
	D_("LAST: walking through service db\n");
	while_active_db(current) {
		/* don't check ourself */
		if (current == service)
			continue;

		/* ignore provided services */
		if (TYPE_PROVIDED && current->type == TYPE_PROVIDED)
			continue;

		/* if this service also should be started last, continue */
		if (is(&LAST, current))
			continue;

		/* TODO, use dep_on_deep */
		/* if current need service that should be last */
		if (initng_depend_deep(current, service) == TRUE) {
			/* don't wait, because this wait is circular */
			D_("Service %s depends on %s\n", service->name,
			   current->name);
			continue;
		}

		if (GET_STATE(current) == IS_STARTING) {
			D_("Service %s is also starting, and %s should be "
			   "started last\n", current->name, service->name);
			event->status = FAILED;
			return;
		}
	}

	/* ok, finally last start this */
}

int module_init(void)
{
	initng_service_data_type_register(&LAST);
	initng_event_hook_register(&EVENT_START_DEP_MET, &check_last);
	return TRUE;
}

void module_unload(void)
{
	initng_service_data_type_unregister(&LAST);
	initng_event_hook_unregister(&EVENT_START_DEP_MET, &check_last);
}
