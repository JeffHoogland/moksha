#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface wl_output_interface;

static const struct wl_interface *types[] = {
	&wl_output_interface,
	&wl_shell_surface_interface,
	&wl_output_interface,
	&wl_shell_surface_interface,
	&wl_shell_surface_interface,
	NULL,
	NULL,
	&wl_shell_surface_interface,
	NULL,
	NULL,
	&wl_shell_surface_interface,
	&wl_output_interface,
};

static const struct wl_message desktop_shell_requests[] = {
	{ "set_background", "oo", types + 0 },
	{ "set_panel", "oo", types + 2 },
	{ "set_lock_surface", "o", types + 4 },
	{ "unlock", "", types + 0 },
};

static const struct wl_message desktop_shell_events[] = {
	{ "configure", "uuoii", types + 5 },
	{ "prepare_lock_surface", "", types + 0 },
};

WL_EXPORT const struct wl_interface desktop_shell_interface = {
	"desktop_shell", 1,
	ARRAY_LENGTH(desktop_shell_requests), desktop_shell_requests,
	ARRAY_LENGTH(desktop_shell_events), desktop_shell_events,
};

static const struct wl_message screensaver_requests[] = {
	{ "set_surface", "oo", types + 10 },
};

WL_EXPORT const struct wl_interface screensaver_interface = {
	"screensaver", 1,
	ARRAY_LENGTH(screensaver_requests), screensaver_requests,
	0, NULL,
};

