/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013 William Light.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/event.h"
#include "rutabaga/object.h"

static const struct rtb_event_handler *find_handler_for(
		rtb_obj_t *obj, rtb_ev_type_t type)
{
	const struct rtb_event_handler *handlers;
	int i, size;

	handlers = obj->handlers->items;
	size = vector_size(obj->handlers);

	for (i = 0; i < size; i++)
		if (handlers[i].type == type)
			return &handlers[i];

	return NULL;
}

int replace_handler(rtb_obj_t *obj, struct rtb_event_handler *with)
{
	const struct rtb_event_handler *handlers;
	int i, size;

	handlers = obj->handlers->items;
	size = vector_size(obj->handlers);

	for (i = 0; i < size; i++) {
		/* if there's already a handler defined for this event type,
		 * replace it. */
		if (handlers[i].type == with->type) {
			vector_set(obj->handlers, i, with);
			return 1;
		}
	}

	return 0;
}

/**
 * public API
 */

int rtb_handle(rtb_obj_t *victim, const rtb_ev_t *ev)
{
	const struct rtb_event_handler *h;

	if (!(h = find_handler_for(victim, ev->type)))
		return 0;

	h->callback.cb(victim, ev, h->callback.ctx);
	return 1;
}

rtb_obj_t *rtb_dispatch_raw(rtb_obj_t *victim, rtb_ev_t *event)
{
	while (!rtb_obj_deliver_event(victim, event) && victim->parent)
		victim = victim->parent;

	return victim;
}

rtb_obj_t *rtb_dispatch_simple(rtb_obj_t *victim, rtb_ev_type_t type)
{
	rtb_ev_t event = {
		.type = type
	};

	return rtb_dispatch_raw(victim, &event);
}

int rtb_attach(rtb_obj_t *victim, rtb_ev_type_t type, rtb_event_cb_t cb,
		void *user_arg)
{
	struct rtb_event_handler handler = {
		.type         = type,
		.callback.cb  = cb,
		.callback.ctx = user_arg
	};

	assert(victim);
	assert(cb);

	if (!replace_handler(victim, &handler))
		vector_push_back(victim->handlers, &handler);

	return 0;
}

void rtb_detach(rtb_obj_t *victim, rtb_ev_type_t type)
{
	const struct rtb_event_handler *handlers;
	int i, size;

	assert(victim);

	handlers = vector_front(victim->handlers);
	size = vector_size(victim->handlers);

	for (i = 0; i < size; i++) {
		if (handlers[i].type == type) {
			vector_erase(victim->handlers, i);
			return;
		}
	}
}
