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

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>

#include "rutabaga/rutabaga.h"
#include "rtb_private/window_impl.h"

#include <GL/wglext.h>

#include "win_rtb.h"

#ifdef _WIN64
# define PRIuSIZE PRIu64
# define PRIxSIZE PRIx64
#else
# define PRIuSIZE PRIu32
# define PRIxSIZE PRIx32
#endif

/**
 * wndproc
 */

static LRESULT CALLBACK
win_rtb_wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct win_rtb_window *self = GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LRESULT ret;

	switch (message) {
	case WM_CREATE:
		PostMessage(hwnd, WM_SHOWWINDOW, 0, 0);
		return 0;

	default:
		if (self) {
			ret = win_rtb_handle_message(self, message, wparam, lparam);

			if (self->need_reconfigure)
				rtb_window_reinit(RTB_WINDOW(self));

			return ret;
		}

		return DefWindowProcW(hwnd, message, wparam, lparam);
	}
}

/**
 * window class bullshit
 */

static ATOM
make_window_class(struct win_rtb_window *self)
{
	wchar_t buf[64];
	WNDCLASSW wc;

	swprintf_s(buf, sizeof(buf) - 1, L"rtb_%" PRIxSIZE, (size_t) self);

	wc = (WNDCLASSW) {
		.style         = CS_OWNDC,
		.lpfnWndProc   = win_rtb_wndproc,

		.cbClsExtra    = 0,
		.cbWndExtra    = 0,
		.hInstance     = 0,

		.hIcon         = LoadIcon(NULL, IDI_APPLICATION),
		.hCursor       = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = NULL,

		.lpszMenuName  = NULL,
		.lpszClassName = buf
	};

	return RegisterClassW(&wc);
}

static void
free_window_class(ATOM window_class)
{
	UnregisterClassW((void *) MAKEINTATOM(window_class), NULL);
}

/**
 * gl context bullshit
 */

struct win_rtb_gl_extensions {
	int create_context_attribs;
	int swap_control;
	int swap_control_tear;
	int appears_to_be_nvidia;
};

static int
check_extensions(struct win_rtb_window *self,
		struct win_rtb_gl_extensions *ext)
{
	PFNWGLGETEXTENSIONSSTRINGEXTPROC get_extensions_string;
	const char *extensions;

	*ext = (struct win_rtb_gl_extensions) {
		.appears_to_be_nvidia   = 0,
		.create_context_attribs = 0,
		.swap_control           = 0,
		.swap_control_tear      = 0
	};

	get_extensions_string = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
		wglGetProcAddress("wglGetExtensionsStringEXT");

	if (!get_extensions_string)
		return -1;

	extensions = get_extensions_string();

	if (strstr(extensions, "WGL_ARB_create_context"))
		ext->create_context_attribs = 1;

	if (strstr(extensions, "WGL_EXT_swap_control")) {
		ext->swap_control = 1;

		if (strstr(extensions, "WGL_EXT_swap_control_tear"))
			ext->swap_control_tear = 1;
	}

	if (strstr(extensions, "NV"))
		ext->appears_to_be_nvidia = 1;

	return 0;
}

static int
init_gl_ctx(struct win_rtb_window *self)
{
	PFNWGLCREATECONTEXTATTRIBSARBPROC create_context_attribs;
	PFNWGLSWAPINTERVALEXTPROC swap_interval;
	struct win_rtb_gl_extensions ext;

	PIXELFORMATDESCRIPTOR pd = {};
	HGLRC gl_ctx;

	const int ctx_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 2,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	pd = (PIXELFORMATDESCRIPTOR) {
		sizeof(pd),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		8,
		8,
		0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	SetPixelFormat(self->dc, ChoosePixelFormat(self->dc, &pd), &pd);

	gl_ctx = wglCreateContext(self->dc);
	if (!gl_ctx)
		goto err_create_ctx;

	wglMakeCurrent(self->dc, gl_ctx);
	check_extensions(self, &ext);

	if (!ext.create_context_attribs)
		goto err_dont_have_create_context_attribs;

	create_context_attribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
		wglGetProcAddress("wglCreateContextAttribsARB");

	wglMakeCurrent(self->dc, NULL);
	wglDeleteContext(gl_ctx);

	if (!create_context_attribs)
		goto err_create_ctx;

	gl_ctx = create_context_attribs(self->dc, NULL, ctx_attribs);
	if (!gl_ctx)
		goto err_create_ctx;

	self->gl_ctx = gl_ctx;
	wglMakeCurrent(self->dc, gl_ctx);

	if (ext.swap_control) {
		swap_interval = (PFNWGLSWAPINTERVALEXTPROC)
			wglGetProcAddress("wglSwapIntervalEXT");

		/* vsync + wgl + nvidia == stupid high CPU usage */
		if (ext.appears_to_be_nvidia)
			swap_interval(0);
		else
			swap_interval(1);
	}

	return 0;

err_dont_have_create_context_attribs:
	wglMakeCurrent(self->dc, NULL);
	wglDeleteContext(gl_ctx);
err_create_ctx:
	return -1;
}

/**
 * window lifecycle
 */

static wchar_t *
utf8_to_utf16_alloc(const rtb_utf8_t *utf8)
{
	wchar_t *buf;
	size_t need;

	need = uv_utf8_to_utf16(utf8, NULL, 0);
	if (!need)
		return NULL;

	buf = calloc(sizeof(*buf), need);
	if (!buf)
		return NULL;

	uv_utf8_to_utf16(utf8, buf, need);
	return buf;
}

struct rtb_window *
window_impl_open(struct rutabaga *r,
		int width, int height, const char *title, intptr_t parent)
{
	struct win_rtb_window *self = calloc(1, sizeof(*self));
	wchar_t *wtitle;
	RECT wrect;
	int flags;

	wtitle = utf8_to_utf16_alloc(title);
	if (!wtitle)
		goto err_wtitle;

	self->window_class = make_window_class(self);
	if (!self->window_class)
		goto err_window_class;

	flags =
		WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE
		| WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	wrect = (RECT) {0, 0, width, height};
	AdjustWindowRectEx(&wrect, flags, FALSE, 0);

	if (parent)
		flags = WS_CHILD;

	self->hwnd = CreateWindowExW((DWORD) 0,
			(void *) MAKEINTATOM(self->window_class), wtitle, flags,
			0, 0,
			wrect.right - wrect.left, wrect.bottom - wrect.top,
			(HWND) parent, NULL, NULL, NULL);

	if (!self->hwnd)
		goto err_createwindow;

	SetWindowLongPtr(self->hwnd, GWLP_USERDATA, (LONG_PTR) self);
	self->dc = GetDC(self->hwnd);

	if (init_gl_ctx(self))
		goto err_gl_ctx;

	/* XXX: hardcode this for now */
	self->dpi.x = 96;
	self->dpi.y = 96;

	free(wtitle);
	return RTB_WINDOW(self);

err_gl_ctx:
	DestroyWindow(self->hwnd);
err_createwindow:
	free_window_class(self->window_class);
err_window_class:
	free(wtitle);
err_wtitle:
	return NULL;
}

void
window_impl_close(struct rtb_window *rwin)
{
	struct win_rtb_window *self = RTB_WINDOW_AS(rwin, win_rtb_window);

	DestroyWindow(self->hwnd);

	free_window_class(self->window_class);
	free(self);
}

/**
 * rutabaga lifecycle
 */

struct rutabaga *
window_impl_rtb_alloc(void)
{
	struct rutabaga *rtb = calloc(1, sizeof(*rtb));
	return rtb;
}

void
window_impl_rtb_free(struct rutabaga *rtb)
{
	free(rtb);
}