#include <assert.h>
#include <stdlib.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/util/log.h>
#include "util/signal.h"

void wlr_primary_selection_source_init(
		struct wlr_primary_selection_source *source,
		const struct wlr_primary_selection_source_impl *impl) {
	assert(impl->send);
	wl_array_init(&source->mime_types);
	wl_signal_init(&source->events.destroy);
	source->impl = impl;
}

void wlr_primary_selection_source_destroy(
		struct wlr_primary_selection_source *source) {
	if (source == NULL) {
		return;
	}

	wlr_signal_emit_safe(&source->events.destroy, source);

	char **p;
	wl_array_for_each(p, &source->mime_types) {
		free(*p);
	}
	wl_array_release(&source->mime_types);

	if (source->impl->destroy) {
		source->impl->destroy(source);
	} else {
		free(source);
	}
}

void wlr_primary_selection_source_send(
		struct wlr_primary_selection_source *source, const char *mime_type,
		int32_t fd) {
	source->impl->send(source, mime_type, fd);
}


void wlr_seat_request_set_primary_selection(struct wlr_seat *seat,
		struct wl_client *client,
		struct wlr_primary_selection_source *source, uint32_t serial) {
	enum ValidationResult res = wlr_serial_validate(seat->serial_tracker,
		client, serial);
	if (res == SERIAL_INVALID) {
		wlr_log(WLR_DEBUG, "Rejecting set_primary_selection request, "
			"serial value %"PRIu32" was never given to the client",
			serial);
		return;
	} else if (res == SERIAL_OLD) {
		wlr_log(WLR_DEBUG, "Beware, very old serial number %"PRIu32" in request, assuming valid",
			serial);
	}

	if (seat->primary_selection_source && serial <= seat->primary_selection_serial) {
		wlr_log(WLR_DEBUG, "Rejecting set_primary_selection request, "
			"serial indicates superseded (%"PRIu32" <= %"PRIu32")",
			serial, seat->primary_selection_serial);
		return;
	}

	struct wlr_seat_request_set_primary_selection_event event = {
		.source = source,
		.serial = serial,
	};
	wlr_signal_emit_safe(&seat->events.request_set_primary_selection, &event);
}

static void seat_handle_primary_selection_source_destroy(
		struct wl_listener *listener, void *data) {
	struct wlr_seat *seat =
		wl_container_of(listener, seat, primary_selection_source_destroy);
	wl_list_remove(&seat->primary_selection_source_destroy.link);
	seat->primary_selection_source = NULL;
	wlr_signal_emit_safe(&seat->events.set_primary_selection, seat);
}

void wlr_seat_set_primary_selection(struct wlr_seat *seat,
		struct wlr_primary_selection_source *source, uint32_t serial) {
	if (seat->primary_selection_source == source) {
		seat->primary_selection_serial = serial;
		return;
	}

	if (seat->primary_selection_source != NULL) {
		wl_list_remove(&seat->primary_selection_source_destroy.link);
		wlr_primary_selection_source_destroy(seat->primary_selection_source);
		seat->primary_selection_source = NULL;
	}

	seat->primary_selection_source = source;
	seat->primary_selection_serial = serial;

	if (source != NULL) {
		seat->primary_selection_source_destroy.notify =
			seat_handle_primary_selection_source_destroy;
		wl_signal_add(&source->events.destroy,
			&seat->primary_selection_source_destroy);
	}

	wlr_signal_emit_safe(&seat->events.set_primary_selection, seat);
}
