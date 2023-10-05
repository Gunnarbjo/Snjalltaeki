// CSTDLIB includes.
#include <stdio.h>
#include <string.h>

// LowNet includes.
#include "lownet.h"

#include "serial_io.h"
#include "utility.h"

#include "app_chat.c"
#include "app_ping.c"

const char* ERROR_OVERRUN = "ERROR // INPUT OVERRUN";
const char* ERROR_UNKNOWN = "ERROR // PROCESSING FAILURE";

const char* ERROR_COMMAND = "Command error";
const char* ERROR_ARGUMENT = "Argument error";

void app_frame_dispatch(const lownet_frame_t* frame) {
	switch(frame->protocol) {
		case LOWNET_PROTOCOL_CHAT:
			chat_receive(frame);
			break;

		case LOWNET_PROTOCOL_PING:
			ping_receive(frame);
			break;
	}
}

void app_main(void)
{
	char msg_in[MSG_BUFFER_LENGTH];
	char msg_out[MSG_BUFFER_LENGTH];
	
	// Initialize the serial services.
	init_serial_service();

	// Initialize the LowNet services.
	lownet_init(app_frame_dispatch);

	while (true) {
		memset(msg_in, 0, MSG_BUFFER_LENGTH);
		memset(msg_out, 0, MSG_BUFFER_LENGTH);

		if (!serial_read_line(msg_in)) {
			// Quick & dirty input parse.  Presume input length > 0.
			if (msg_in[0] == '/') {
				// Some kind of command.
				//	...?
			} else if (msg_in[0] == '@') {
				// Probably a chat 'tell' command.
				//	...?
				chat_tell(/* ...? */);
			} else {
				// Default, chat broadcast message.
				//	...?
				chat_shout(msg_in);
			}
		}
	}
}
