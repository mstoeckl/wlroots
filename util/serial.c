#define _POSIX_C_SOURCE 200112L

#include <wlr/util/log.h>
#include <wlr/util/serial.h>
#include <stdlib.h>

#include <wayland-server-core.h>

// use much smaller values for testing
#define WLR_SERIAL_TRACKER_SIZE 4096

struct wlr_serial_tracker {
	/* TODO: replace wl_client with a global client-counter which is not
	 * susceptible to the ABA problem. */
	struct wl_client *ring_buffer[WLR_SERIAL_TRACKER_SIZE];
	int end;
	int count;
	uint32_t last_serial;
	struct wl_display *display;
};

struct wlr_serial_tracker *create_wlr_serial_tracker(struct wl_display *display) {
	struct wlr_serial_tracker *tracker = calloc(1, sizeof(struct wlr_serial_tracker));
	tracker->last_serial = wl_display_get_serial(display);
	tracker->count = 0;
	tracker->display = display;
	return tracker;
}

void wlr_serial_tracker_destroy(struct wlr_serial_tracker *wlr_serial_tracker) {
	free(wlr_serial_tracker);
}


uint32_t wlr_serial_next(struct wlr_serial_tracker *tracker,
			 struct wl_client *client) {
	uint32_t nxt_serial = wl_display_next_serial(tracker->display);

	tracker->count += nxt_serial - tracker->last_serial;
	if (tracker->count > WLR_SERIAL_TRACKER_SIZE) {
		tracker->count = WLR_SERIAL_TRACKER_SIZE;
	}
	if (nxt_serial >= tracker->last_serial + WLR_SERIAL_TRACKER_SIZE) {
		/* Special case, wipe the board */
		memset(tracker->ring_buffer, 0, sizeof(tracker->ring_buffer));
		tracker->last_serial = nxt_serial;
		tracker->ring_buffer[0] = client;
		tracker->end = 0;
	} else {
		int new_end = tracker->end + (int)(nxt_serial - tracker->last_serial);
		/* We zero out intervening values, so that all queries to them
		 * will return invalid */
		new_end = new_end % WLR_SERIAL_TRACKER_SIZE;
		if (new_end < tracker->end) {
			/* wraparound case */
			memset(&tracker->ring_buffer[tracker->end], 0,
				sizeof(struct wl_client*) *
				(WLR_SERIAL_TRACKER_SIZE - tracker->end));
			memset(&tracker->ring_buffer, 0,
				sizeof(struct wl_client*) * new_end);
		} else {
			memset(&tracker->ring_buffer[tracker->end + 1], 0,
				sizeof(struct wl_client*) *
				(new_end - tracker->end - 1));
		}
		tracker->ring_buffer[new_end] = client;
		tracker->end = new_end;
		tracker->last_serial = nxt_serial;

		// wlr_log(WLR_DEBUG, "Allocating serial %u for client %lx, writing to %d", nxt_serial, (uint64_t)client, new_end);

	}


	return nxt_serial;
}

enum ValidationResult wlr_serial_validate(struct wlr_serial_tracker *tracker,
	struct wl_client *client, uint32_t serial) {
	uint32_t relative_sub = tracker->last_serial - serial;
	if (relative_sub > UINT32_MAX / 2) {
		return SERIAL_INVALID;
	}
	if (relative_sub >= (uint32_t)tracker->count) {
		return SERIAL_OLD;
	}
	int index = tracker->end - (int)relative_sub + WLR_SERIAL_TRACKER_SIZE;
	index = index % WLR_SERIAL_TRACKER_SIZE;
	// wlr_log(WLR_DEBUG, "Checking serial %u for client %lx at position %d, got %lx", serial, (uint64_t)client, index, (uint64_t)tracker->ring_buffer[index] );
	if (tracker->ring_buffer[index] == client) {
		return SERIAL_VALID;
	} else {
		return SERIAL_INVALID;
	}
}
