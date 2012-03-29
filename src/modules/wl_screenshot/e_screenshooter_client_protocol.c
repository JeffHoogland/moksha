#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_buffer_interface;

static const struct wl_interface *types[] = {
	&wl_output_interface,
	&wl_buffer_interface,
};

static const struct wl_message screenshooter_requests[] = {
	{ "shoot", "oo", types + 0 },
};

WL_EXPORT const struct wl_interface screenshooter_interface = {
	"screenshooter", 1,
	ARRAY_LENGTH(screenshooter_requests), screenshooter_requests,
	0, NULL,
};

