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

/**
 * Borrowed from @ofurtumi to check if ping works  
 * Function to print the ping information,
 * we know the packet is valid so we start by printing
 * what we received, then what we sent back
 * @param frame - The frame containing the ping packet
 */
void ping_info(const lownet_frame_t *frame, const lownet_frame_t *reply) {
  char rec_msg[19 + 8 + 8 + 8];
  sprintf(rec_msg, "Ping from 0x%x to 0x%x at %lu:%d\n", frame->source,
          frame->destination,
          ((ping_packet_t *)&frame->payload)->timestamp_out.seconds,
          ((ping_packet_t *)&frame->payload)->timestamp_out.parts);
  serial_write_line(rec_msg);

  char rep_msg[16 + 8 + 8];
  sprintf(rep_msg, "Replied to %x at %lu.%d\n", reply->destination,
          ((ping_packet_t *)&reply->payload)->timestamp_back.seconds,
          ((ping_packet_t *)&reply->payload)->timestamp_back.parts);
  serial_write_line(rep_msg);
}

/**
 * Initiates a ping to a specified node in the low-net network.
 * 
 * This function constructs a ping packet with the current network
 * time and device ID, then sends the packet to the specified node.
 * 
 * @param node ID of the destination node.
 */
void ping(uint8_t node) {
  ping_packet_t packet;
  packet.timestamp_out = lownet_get_time();
  packet.origin = lownet_get_device_id();

  lownet_frame_t frame;
  frame.source = lownet_get_device_id();
  frame.destination = node;
  frame.protocol = LOWNET_PROTOCOL_PING;
  frame.length = sizeof(ping_packet_t);
  
  // Ensure frame's payload can hold the packet.
  if (sizeof(frame.payload) >= frame.length) {
    memcpy(&frame.payload, &packet, frame.length);
    lownet_send(&frame);
  } else {
    // Handle the error - payload too large for frame.
    serial_write_line("ERROR: Ping packet is too large for frame payload.");
  }
}

/**
 * Handles the reception of a ping packet.
 * 
 * When a ping packet is received, this function checks if it is 
 * correctly formed. If the current device is the intended receiver
 * (or if it's a broadcast packet), a reply is sent back.
 * 
 * @param frame The received frame containing the ping packet.
 */
void ping_receive(const lownet_frame_t* frame) {
  // Needs this printline to work, DO NOT TOUCH!!!!
  char buf[20];
  sprintf(buf, "t: %ul %ul\n", frame->length, sizeof(ping_packet_t));
  serial_write_line(buf);

  if (frame->length < sizeof(ping_packet_t)) {
    // Malformed frame.  Discard.
    return;
  }

  //    serial_write_line("pong");
  uint8_t receiver = frame->destination;
 
  if (receiver == lownet_get_device_id() || receiver == 0xFF) {
    // serial_write_line("gottem!");
    if(((ping_packet_t*) frame->payload)->origin != lownet_get_device_id()){
      // serial_write_line("all!");
	lownet_frame_t reply;
	reply.source = lownet_get_device_id();
	reply.destination = frame->source;
	reply.protocol = frame->protocol;
	memcpy(&reply.payload, frame->payload, sizeof(ping_packet_t));
	((ping_packet_t *)&reply.payload)->timestamp_back = lownet_get_time();
	lownet_send(&reply);
      }
    else serial_write_line("You got mail");
    
  }
}
