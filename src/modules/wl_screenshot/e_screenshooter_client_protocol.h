#ifndef E_SCREENSHOOTER_CLIENT_PROTOCOL_H
#define E_SCREENSHOOTER_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

extern const struct wl_interface screenshooter_interface;

#define SCREENSHOOTER_SHOOT	0

static inline void
screenshooter_set_user_data(E_Screenshooter *screenshooter, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) screenshooter, user_data);
}

static inline void *
screenshooter_get_user_data(E_Screenshooter *screenshooter)
{
	return wl_proxy_get_user_data((struct wl_proxy *) screenshooter);
}

static inline void
screenshooter_destroy(E_Screenshooter *screenshooter)
{
	wl_proxy_destroy((struct wl_proxy *) screenshooter);
}

static inline void
screenshooter_shoot(E_Screenshooter *screenshooter, struct wl_output *output, struct wl_buffer *buffer)
{
	wl_proxy_marshal((struct wl_proxy *) screenshooter,
			 SCREENSHOOTER_SHOOT, output, buffer);
}

#ifdef  __cplusplus
}
#endif

#endif
