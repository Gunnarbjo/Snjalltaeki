
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_ping.h"

#include "serial_io.h"

typedef struct __attribute__((__packed__))
{
	lownet_time_t 	timestamp_out;
	lownet_time_t	timestamp_back;
	uint8_t 		origin;
} ping_packet_t;


void ping(uint8_t node) {
	// ...?
}


void ping_receive(const lownet_frame_t* frame) {
	if (frame->length < sizeof(ping_packet_t)) {
		// Malformed frame.  Discard.
		return;
	}

	// ...?
}