
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <string.h>

#include "lownet.h"
#include "serial_io.h"
#include "utility.h"

#include "app_chat.h"


/**
 * Determines whether a given message can be sent based on its content and size.
 *
 * This function checks for two main constraints:
 * 1. The length of the message does not exceed LOWNET_PAYLOAD_SIZE.
 * 2. All characters in the message are printable.
 * 
 * @param message The message string to check.
 * @return true if the message is eligible for sending, false otherwise.
 */
bool can_send(const char *message) {
    if (!message) return false;

    int i = 0;
    
    for (; message[i]; i++) {
        if (!util_printable(message[i])) return false;
    }
    if(i > LOWNET_PAYLOAD_SIZE) return false;

    return true;
}

/**
 * Handles the reception of chat messages from other nodes.
 *
 * This function processes the frame payload to extract and display
 * the chat message. Depending on the destination address, the message
 * might be a direct message or a broadcast (shout).
 *
 * @param frame The received lownet frame containing the chat message.
 */
void chat_receive(const lownet_frame_t* frame) {
    char prefix[20];
    if (frame->destination == lownet_get_device_id()) {
        sprintf(prefix, "Message from 0x%x: ", frame->source);
    } else if (frame->destination == 0xFF) {
        sprintf(prefix, "Shout from 0x%x: ", frame->source);
    } else {
        return;
    }
    serial_write_line(prefix);

    char message[frame->length + 1];
    memcpy(message, frame->payload, frame->length);
    message[frame->length] = '\0';
    serial_write_line(message);
}

/**
 * Constructs and sends a chat message in a lownet frame.
 *
 * This is a helper function to minimize code repetition when sending
 * chat messages, be it a direct message or a broadcast.
 *
 * @param message The chat message to send.
 * @param destination The destination node address.
 */
void send_frame(const char* message, uint8_t destination) {
    size_t length = strlen(message);

    lownet_frame_t frame;
    frame.source = lownet_get_device_id();
    frame.destination = destination;
    frame.protocol = LOWNET_PROTOCOL_CHAT;
    frame.length = length;

    memcpy(frame.payload, message, length);
    frame.payload[length] = '\0';

    lownet_send(&frame);
}

/**
 * Broadcasts a chat message to all nodes.
 *
 * This function is used to shout a message, i.e., send it to all nodes.
 * It first checks the eligibility of the message using can_send() and
 * then proceeds to send it if eligible.
 *
 * @param message The message to broadcast.
 */
void chat_shout(const char* message) {
    if (can_send(message)) {
        send_frame(message, 0xFF);
    }
}

/**
 * Sends a direct chat message to a specified node.
 *
 * This function sends a message to a single, specified node. Before
 * sending, it checks the eligibility of the message using can_send().
 *
 * @param message The message to send.
 * @param destination The destination node address.
 */
void chat_tell(const char* message, uint8_t destination) {
    if (can_send(message)) {
        send_frame(message, destination);
    }
}
