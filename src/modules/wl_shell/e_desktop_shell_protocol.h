#ifndef DESKTOP_SERVER_PROTOCOL_H
#define DESKTOP_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

struct desktop_shell;
struct screensaver;

extern const struct wl_interface desktop_shell_interface;
extern const struct wl_interface screensaver_interface;

struct desktop_shell_interface {
	/**
	 * set_background - (none)
	 * @output: (none)
	 * @surface: (none)
	 */
	void (*set_background)(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *output,
			       struct wl_resource *surface);
	/**
	 * set_panel - (none)
	 * @output: (none)
	 * @surface: (none)
	 */
	void (*set_panel)(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *output,
			  struct wl_resource *surface);
	/**
	 * set_lock_surface - (none)
	 * @surface: (none)
	 */
	void (*set_lock_surface)(struct wl_client *client,
				 struct wl_resource *resource,
				 struct wl_resource *surface);
	/**
	 * unlock - (none)
	 */
	void (*unlock)(struct wl_client *client,
		       struct wl_resource *resource);
};

#define DESKTOP_SHELL_CONFIGURE	0
#define DESKTOP_SHELL_PREPARE_LOCK_SURFACE	1

static inline void
desktop_shell_send_configure(struct wl_resource *resource_, uint32_t time, uint32_t edges, struct wl_resource *surface, int32_t width, int32_t height)
{
	wl_resource_post_event(resource_, DESKTOP_SHELL_CONFIGURE, time, edges, surface, width, height);
}

static inline void
desktop_shell_send_prepare_lock_surface(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, DESKTOP_SHELL_PREPARE_LOCK_SURFACE);
}

struct screensaver_interface {
	/**
	 * set_surface - (none)
	 * @surface: (none)
	 * @output: (none)
	 */
	void (*set_surface)(struct wl_client *client,
			    struct wl_resource *resource,
			    struct wl_resource *surface,
			    struct wl_resource *output);
};

#ifdef  __cplusplus
}
#endif

#endif
