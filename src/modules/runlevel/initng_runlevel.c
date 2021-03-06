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

/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTION DEFINES                  #
 * ############################################################################
 */
static int start_RUNLEVEL(active_db_h * service_to_start);
static int stop_RUNLEVEL(active_db_h * service);

/*
 * ############################################################################
 * #                        STATE HANDLERS FUNCTION DEFINES                   #
 * ############################################################################
 */
static void init_RUNLEVEL_START_MARKED(active_db_h * service);
static void init_RUNLEVEL_STOP_MARKED(active_db_h * service);
static void handle_RUNLEVEL_WAITING_FOR_START_DEP(active_db_h * service);
static void handle_RUNLEVEL_WAITING_FOR_STOP_DEP(active_db_h * service);

/*
 * ############################################################################
 * #                     Official SERVICE_TYPE STRUCT                         #
 * ############################################################################
 */
stype_h TYPE_RUNLEVEL = {
	.name = "runlevel",
	.description = "A runlevel contains a set of services that shud be "
	    "running on the system.",
	.hidden = FALSE,
	.start = &start_RUNLEVEL,
	.stop = &stop_RUNLEVEL,
	.restart = NULL
};

stype_h TYPE_VIRTUAL = {
	.name = "virtual",
	.description =
	    "A virtual is a virtual set of services with an unik group "
	    "name.",
	.hidden = TRUE,
	.start = &start_RUNLEVEL,
	.stop = &stop_RUNLEVEL,
	.restart = NULL
};

/*
 * ############################################################################
 * #                         SERVICE STATES STRUCTS                           #
 * ############################################################################
 */

/*
 * When we want to start a service, it is first RUNLEVEL_START_MARKED
 */
a_state_h RUNLEVEL_START_MARKED = {
	.name = "START_MARKED",
	.description = "This runlevel is marked to be started.",
	.is = IS_STARTING,
	.interrupt = NULL,
	.init = &init_RUNLEVEL_START_MARKED,
	.alarm = NULL
};

/*
 * When we want to stop a SERVICE_DONE service, its marked
 * RUNLEVEL_STOP_MARKED
 */
a_state_h RUNLEVEL_STOP_MARKED = {
	.name = "STOP_MARKED",
	.description = "This runlevel is marked to be stopped.",
	.is = IS_STOPPING,
	.interrupt = NULL,
	.init = &init_RUNLEVEL_STOP_MARKED,
	.alarm = NULL
};

/*
 * When a service is UP, it is marked as RUNLEVEL_UP
 */
a_state_h RUNLEVEL_UP = {
	.name = "UP",
	.description = "This runlevel is UP.",
	.is = IS_UP,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * When services needed by current one is starting, current service is set
 * RUNLEVEL_WAITING_FOR_START_DEP
 */
a_state_h RUNLEVEL_WAITING_FOR_START_DEP = {
	.name = "WAITING_FOR_START_DEP",
	.description = "Waiting for all services in this runlevel to start.",
	.is = IS_STARTING,
	.interrupt = &handle_RUNLEVEL_WAITING_FOR_START_DEP,
	.init = NULL,
	.alarm = NULL
};

/*
 * When services needed to stop, before this is stopped is stopping, current
 * service is set RUNLEVEL_WAITING_FOR_STOP_DEP
 */
a_state_h RUNLEVEL_WAITING_FOR_STOP_DEP = {
	.name = "WAITING_FOR_STOP_DEP",
	.description = "Waiting for all services depending on this runlevel "
	    "to stop, before the runlevel can be stopped.",
	.is = IS_STOPPING,
	.interrupt = &handle_RUNLEVEL_WAITING_FOR_STOP_DEP,
	.init = NULL,
	.alarm = NULL
};

/*
 * This marks the services, as DOWN.
 */
a_state_h RUNLEVEL_DOWN = {
	.name = "DOWN",
	.description = "Runlevel is not currently active.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * Generally FAILING states, if something goes wrong, some of these are set.
 */
a_state_h RUNLEVEL_START_DEPS_FAILED = {
	.name = "START_DEPS_FAILED",
	.description = "Some services in this runlevel failed",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

/*
 * ############################################################################
 * #                         STYPE HANDLERS FUNCTIONS                         #
 * ############################################################################
 */

/* This are run, when initng wants to start a service */
static int start_RUNLEVEL(active_db_h * service)
{
	/* if not yet stopped */
	if (IS_MARK(service, &RUNLEVEL_WAITING_FOR_STOP_DEP)) {
		initng_common_mark_service(service, &RUNLEVEL_UP);
		return TRUE;
	}

	/* mark it WAITING_FOR_START_DEP and wait */
	if (!initng_common_mark_service(service, &RUNLEVEL_START_MARKED)) {
		W_("mark_service RUNLEVEL_START_MARKED failed for service "
		   "%s\n", service->name);
		return FALSE;
	}

	/* return happily */
	return TRUE;
}

/* This are run, when initng wants to stop a service */
static int stop_RUNLEVEL(active_db_h * service)
{
	/* if not yet stopped */
	if (IS_MARK(service, &RUNLEVEL_WAITING_FOR_START_DEP)) {
		initng_common_mark_service(service, &RUNLEVEL_DOWN);
		return TRUE;
	}

	/* set stopping */
	if (!initng_common_mark_service(service, &RUNLEVEL_STOP_MARKED)) {
		W_("mark_service RUNLEVEL_STOP_MARKED failed for service "
		   "%s.\n", service->name);
		return FALSE;
	}

	/* return happily */
	return TRUE;
}

/*
 * ############################################################################
 * #                          MODULE INITIATORS                               #
 * ############################################################################
 */

int module_init(void)
{
	initng_service_type_register(&TYPE_RUNLEVEL);
	initng_service_type_register(&TYPE_VIRTUAL);

	initng_active_state_register(&RUNLEVEL_START_MARKED);
	initng_active_state_register(&RUNLEVEL_STOP_MARKED);
	initng_active_state_register(&RUNLEVEL_UP);
	initng_active_state_register(&RUNLEVEL_WAITING_FOR_START_DEP);
	initng_active_state_register(&RUNLEVEL_WAITING_FOR_STOP_DEP);
	initng_active_state_register(&RUNLEVEL_DOWN);
	initng_active_state_register(&RUNLEVEL_START_DEPS_FAILED);

	return TRUE;
}

void module_unload(void)
{
	initng_service_type_unregister(&TYPE_RUNLEVEL);
	initng_service_type_unregister(&TYPE_VIRTUAL);

	initng_active_state_unregister(&RUNLEVEL_START_MARKED);
	initng_active_state_unregister(&RUNLEVEL_STOP_MARKED);
	initng_active_state_unregister(&RUNLEVEL_UP);
	initng_active_state_unregister(&RUNLEVEL_WAITING_FOR_START_DEP);
	initng_active_state_unregister(&RUNLEVEL_WAITING_FOR_STOP_DEP);
	initng_active_state_unregister(&RUNLEVEL_DOWN);
	initng_active_state_unregister(&RUNLEVEL_START_DEPS_FAILED);
}

/*
 * ############################################################################
 * #                         STATE_FUNCTIONS                                  #
 * ############################################################################
 */

static inline active_db_h *runlevel_find_old(active_db_h *new_rl)
{
	active_db_h *current = NULL;

	while_active_db(current) {
		if (current->type == &TYPE_RUNLEVEL && current != new_rl)
			return current;
	}
	return NULL;
}

static inline int runlevel_find_dep(active_db_h *rl, const char *dep)
{
	const char *dep_new;
	s_data *itt_new = NULL;

	while ((dep_new = get_next_string(&NEED, rl, &itt_new))) {
		if (strcmp(dep_new, dep) == 0)
			return TRUE;
	}

	return FALSE;
}

static inline void runlevel_stop_unneeded(active_db_h *old_rl, active_db_h *new_rl)
{
	const char *dep_old = NULL;
	s_data *itt_old = NULL;

	/* for every dep the old runlevel had */
	while ((dep_old = get_next_string(&NEED, old_rl, &itt_old))) {
		/* check if required by the new one */
		int found = runlevel_find_dep(new_rl, dep_old);

		/* and if not, stop it */
		if (!found) {
			active_db_h *to_stop =
				initng_active_db_find_by_name(dep_old);
			if (to_stop) {
				W_("Stopping service %s, not "
				   "in the '%s' runlevel\n",
				   to_stop->name, new_rl->name);
				initng_handler_stop_service(to_stop);
			}
		}
	}
}

/*
 * Everything RUNLEVEL_START_MARKED are gonna do, is to set it RUNLEVEL_WAITING_FOR_START_DEP
 */
static void init_RUNLEVEL_START_MARKED(active_db_h * new_runlevel)
{
	/* Start our dependencies */
	initng_depend_start_deps(new_runlevel);

	/* Make sure there will only exist 1 runlevel on the system */
	if (new_runlevel->type == &TYPE_RUNLEVEL) {
		active_db_h *old_runlevel = runlevel_find_old(new_runlevel);
		if (old_runlevel) {
			/* Stop all old runlevel deps, that are not deps of the
			   new runlevel */
			runlevel_stop_unneeded(old_runlevel, new_runlevel);
			/* Finally, stop the old runlevel */
			initng_handler_stop_service(old_runlevel);
		}

		free(g.runlevel);

		g.runlevel = initng_toolbox_strdup(new_runlevel->name);
	}

	initng_common_mark_service(new_runlevel,
				   &RUNLEVEL_WAITING_FOR_START_DEP);
}

/*
 * Everything RUNLEVEL_STOP_MARKED are gonna do, is to set it RUNLEVEL_WAITING_FOR_STOP_DEP
 */
static void init_RUNLEVEL_STOP_MARKED(active_db_h * service)
{
	/* Stop all services dependeing on this service */
	initng_depend_stop_deps(service);

	initng_common_mark_service(service, &RUNLEVEL_WAITING_FOR_STOP_DEP);
}

static void handle_RUNLEVEL_WAITING_FOR_START_DEP(active_db_h * service)
{
	assert(service);

	/* this checks with external plug-ins, if its ok to start this service now */

	switch (initng_depend_start_dep_met(service, FALSE)) {
		/* if not met, youst return */
	case FALSE:
		return;

		/* set FAILURE */
	case FAIL:
		initng_common_mark_service(service,
					   &RUNLEVEL_START_DEPS_FAILED);
		return;

		/* if met, continue */
	case TRUE:
	default:
		break;
	}

	/* set status to START_DEP_MET */
	initng_common_mark_service(service, &RUNLEVEL_UP);
}

static void handle_RUNLEVEL_WAITING_FOR_STOP_DEP(active_db_h * service)
{
	assert(service);

	/* check with other plug-ins, if it is ok to stop this service now */
	switch (initng_depend_stop_dep_met(service, FALSE)) {
		/* deps not met, youst return */
	case FALSE:
		return;

		/* if met, youst continue */
	case TRUE:
	default:
		break;
	}

	/* ok, stopping deps are met */
	initng_common_mark_service(service, &RUNLEVEL_DOWN);
}
