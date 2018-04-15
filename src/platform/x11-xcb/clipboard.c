/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2017 William Light.
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

#include <rutabaga/window.h>
#include <rutabaga/platform.h>

#include "xrtb.h"

void
rtb_copy_to_clipboard(struct rtb_window *rwin, const rtb_utf8_t *buf,
		size_t nbytes)
{
	struct xrtb_window *self = RTB_WINDOW_AS(rwin, xrtb_window);
	struct xcb_rutabaga *xrtb = self->xrtb;

	free(xrtb->clipboard.buffer);
	xrtb->clipboard.buffer = strndup(buf, nbytes);
	xrtb->clipboard.nbytes = nbytes;

	if (!xrtb->clipboard.buffer)
		return;

	xcb_set_selection_owner(xrtb->xcb_conn, self->xcb_win,
			xrtb->atoms.clipboard, XCB_CURRENT_TIME);
	xcb_flush(xrtb->xcb_conn);
}
