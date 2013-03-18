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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>

#include <math.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/object.h"
#include "rutabaga/window.h"
#include "rutabaga/render.h"
#include "rutabaga/layout.h"

#include "private/util.h"
#include "rutabaga/widgets/button.h"

#define SELF_FROM(obj) rtb_button_t *self = RTB_BUTTON_T(obj)

#define BACKGROUND_NORMAL	RGB(0x404F3C)
#define BACKGROUND_HOVER	RGB(0x5C704C)
#define BACKGROUND_FOCUS	RGB(0x364232)

#define OUTLINE_NORMAL		RGB(0x404F3C)
#define OUTLINE_HOVER		OUTLINE_NORMAL
#define OUTLINE_FOCUS		BACKGROUND_FOCUS

#define TEXT_NORMAL			RGB(0xFFFFFF)

static struct rtb_object_implementation super;

/**
 * drawing-related things
 */

static const GLubyte box_indices[] = {
	0, 1, 3, 2
};

static void cache_to_vbo(rtb_button_t *self)
{
	GLfloat x, y, w, h, box[4][2];

	x = self->x;
	y = self->y;
	w = self->w;
	h = self->h;

	box[0][0] = x;
	box[0][1] = y;

	box[1][0] = x + w;
	box[1][1] = y;

	box[2][0] = x + w;
	box[2][1] = y + h;

	box[3][0] = x;
	box[3][1] = y + h;

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void draw(rtb_obj_t *obj, rtb_draw_state_t state)
{
	SELF_FROM(obj);

	rtb_render_push(self);
	rtb_render_set_position(self, 0, 0);

	switch (state) {
	case RTB_DRAW_NORMAL:
		rtb_render_set_color(self, BACKGROUND_NORMAL, 1.f);
		break;

	case RTB_DRAW_HOVER:
		rtb_render_set_color(self, BACKGROUND_HOVER, 1.f);
		break;

	case RTB_DRAW_FOCUS:
		rtb_render_set_color(self, BACKGROUND_FOCUS, 1.f);
		break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawElements(
			GL_TRIANGLE_STRIP, ARRAY_LENGTH(box_indices),
			GL_UNSIGNED_BYTE, box_indices);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	super.draw_cb(obj, state);
}

/**
 * event handlers
 */

static int dispatch_click_event(rtb_button_t *self, const rtb_ev_mouse_t *e)
{
	rtb_ev_button_t event = *((rtb_ev_button_t *) e);

	event.type = BUTTON_CLICK;
	event.cursor.x -= self->rect.p1.x;
	event.cursor.y -= self->rect.p1.y;

	return rtb_handle(self, RTB_EV_T(&event));
}

static int on_event(rtb_obj_t *obj, const rtb_ev_t *e)
{
	SELF_FROM(obj);

	switch (e->type) {
	case RTB_MOUSE_DOWN:
		/* XXX: HACK */
		self->window->focus = self;
	case RTB_MOUSE_ENTER:
	case RTB_MOUSE_UP:
	case RTB_MOUSE_LEAVE:
	case RTB_DRAG_DROP:
		rtb_obj_mark_dirty(self);
		break;

	case RTB_DRAG_START:
		return 1;

	case RTB_MOUSE_CLICK:
		if (RTB_EV_MOUSE_T(e)->button != RTB_MOUSE_BUTTON1)
			return 0;

		return dispatch_click_event(self, RTB_EV_MOUSE_T(e));

	default:
		return super.event_cb(obj, e);
	}

	return 0;
}

static void recalculate(rtb_obj_t *obj, rtb_obj_t *instigator,
		rtb_event_direction_t direction)
{
	SELF_FROM(obj);

	super.recalc_cb(obj, instigator, direction);
	cache_to_vbo(self);
}

static void realize(rtb_obj_t *obj, rtb_obj_t *parent, rtb_win_t *window)
{
	SELF_FROM(obj);

	super.realize_cb(obj, parent, window);
	self->type = rtb_type_ref(window, self->type,
			"net.illest.rutabaga.widgets.button");

	self->outer_pad.x = self->label.outer_pad.x;
	self->outer_pad.y = self->label.outer_pad.y;

	cache_to_vbo(self);
}

/**
 * public API
 */

void rtb_button_set_label(rtb_button_t *self, const wchar_t *label)
{
	rtb_label_set_text(&self->label, label);
}

rtb_button_t *rtb_button_new(const wchar_t *label)
{
	rtb_button_t *self = calloc(1, sizeof(rtb_button_t));
	rtb_obj_init(self, &super);

	rtb_label_init(&self->label, &self->label);
	rtb_obj_add_child(self, &self->label, RTB_ADD_HEAD);

	glGenBuffers(1, &self->vbo);

	if (label)
		rtb_button_set_label(self, label);

	self->outer_pad.x =
		self->outer_pad.y = 0.f;

	self->min_size.w = 70.f;
	self->min_size.h = 26.f;

	self->draw_cb    = draw;
	self->event_cb   = on_event;
	self->realize_cb = realize;
	self->layout_cb  = rtb_layout_hpack_center;
	self->size_cb    = rtb_size_hfit_children;
	self->recalc_cb  = recalculate;

	return self;
}

void rtb_button_free(rtb_button_t *self)
{
	rtb_label_fini(&self->label);
	rtb_obj_fini(self);
	free(self);
}
