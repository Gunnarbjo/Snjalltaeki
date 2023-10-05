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

char buffer[128];

/**
 * Fetches the network time, formats it as a string, and returns it.
 * 
 * @return A pointer to the formatted network time string.
 */
const char* get_network_time_string() {
  lownet_time_t network_time = lownet_get_time();

  if (network_time.seconds == 0 && network_time.parts == 0) {
    // Network time not available.
    snprintf(buffer, sizeof(buffer), "TIME ERROR: Network time not available.");
  } else {
    float fractional_seconds = (float)network_time.parts / 256.0;
    snprintf(buffer, sizeof(buffer), "Network Time: %lu seconds + %f seconds", network_time.seconds, fractional_seconds);
  }

  return buffer;
}

/**
 * Main application loop. Initializes necessary services, waits for user input, 
 * parses the input, and dispatches commands or sends messages accordingly.
 */
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
      // Quick & dirty input parse.
      if(msg_in[0] == 0) continue;
      
      if (msg_in[0] == '/') {

	// we spilt the string and get the command
	char *cmd = strtok(msg_in + 1, " ");
	char *arg = strtok(NULL, " ");

	if(!strcmp(cmd,"ping")){
	  if(arg){
	    uint8_t destination = (uint8_t)hex_to_dec(arg + 2);

	    if(destination == 0){
	      serial_write_line("PING ERROR: The node 0x0 is invalid\n");
	      continue;
	    }
	    ping(destination); 
	  }
	  else{
	    serial_write_line("PING ERROR: No valid node provided\n");
	  }
	}
	
	else if(!strcmp(cmd,"date")){
	  get_network_time_string();
	}
      }	
	
    
      else if (msg_in[0] == '@') {
	char *destination = strtok(msg_in + 1, " ");
	char *message = strtok(NULL, "\n");

	if(!message){
	  serial_write_line("MESSAGE ERROR: No message provided\n");
	  continue;
	}

	uint8_t destinationAddress = (uint8_t)hex_to_dec(destination + 2);

	if(destinationAddress == 0){
	  serial_write_line("CHAT ERROR: 0x0 is not valid address");
	  continue;
	}
	
	
	chat_tell(message,destinationAddress);
      }
      else {
	// Default, chat broadcast message.
	chat_shout(msg_in);
      }
    }
  }
}


