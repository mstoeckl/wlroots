/*
 * This an unstable interface of wlroots. No guarantees are made regarding the
 * future consistency of this API.
 */
#ifndef WLR_USE_UNSTABLE
#error "Add -DWLR_USE_UNSTABLE to enable unstable wlroots features"
#endif

#ifndef WLR_UTIL_SERIAL_H
#define WLR_UTIL_SERIAL_H

#include <stdint.h>

struct wl_client;
struct wl_display;

/**
 * This struct contains the state tracking mechanisms used to verify that
 * a serial value returned by the client actually was passed to the client
 * to begin with.
 *
 * Serial values are used to ensure that the client which was interacted
 * with last was priority for selection, grab, and other shared resources.
 * If a client makes requests with serials that were not passed to it, then
 * it can interfere with other applications.
 *
 * Use this interface instead of wl_display_next_serial and wl_display_get_serial.
 *
 * To save memory, this serial tracker only validates the last few thousand
 * serial numbers; all previous numbers are hopefully old enough to not be
 * usable for stealing any grabs.
 */
struct wlr_serial_tracker;

/**
 * Allocates a new wlr_serial_tracker associated with the display *display*.
 */
struct wlr_serial_tracker *create_wlr_serial_tracker(struct wl_display *display);
/**
 * Destroys a wlr_seat and removes its wl_seat global.
 */
void wlr_serial_tracker_destroy(struct wlr_serial_tracker *wlr_serial_tracker);


/**
 * Increments the display serial number, updates the serial number tracker, and
 * and returns the new value.
 */
#define XWAYLAND_SERIAL_CLIENT (struct wl_client *)1
uint32_t wlr_serial_next(struct wlr_serial_tracker *tracker,
	struct wl_client *client);


/**
 * Confirms that the serial number provided was actually send to the client.
 * If 'SERIAL_INVALID' is returned, then (if wlr_serial_tracker was used
 * consistently) the serial number was definitely not provided to the client. If
 * 'SERIAL_VALID' is returned, then it definitely was; and if 'SERIAL_OLD' is
 * returned then the serial value is old enough that the tracker no longer
 * can tell.
 */
enum ValidationResult {SERIAL_INVALID = -1, SERIAL_VALID = 0, SERIAL_OLD = 1};
enum ValidationResult wlr_serial_validate(struct wlr_serial_tracker *tracker,
	struct wl_client *client, uint32_t serial);

#endif
