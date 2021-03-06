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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};


s_entry CHDIR = {
	.name = "chdir",
	.description = "Change to this directory before launching.",
	.type = STRING,
	.ot = NULL,
};

static void do_chdir(s_event * event)
{
	s_event_after_fork_data *data;

	const char *tmp = NULL;

	assert(event->event_type == &EVENT_AFTER_FORK);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->service->name);
	assert(data->process);

	D_("do_chdir!\n");

	if (!(tmp = get_string(&CHDIR, data->service))) {
		D_("CHDIR not set!\n");
		return;
	}

	D_("CHDIR TO %s\n", tmp);

	if (chdir(tmp) == -1) {
		F_("Chdir failed with %s\n", strerror(errno));
		event->status = FAILED;
	}
}

int module_init(void)
{
	initng_service_data_type_register(&CHDIR);
	return (initng_event_hook_register(&EVENT_AFTER_FORK, &do_chdir));
}

void module_unload(void)
{
	initng_service_data_type_unregister(&CHDIR);
	initng_event_hook_unregister(&EVENT_AFTER_FORK, &do_chdir);
}
