// CSTDLIB includes.
#include <ctype.h>
#include <string.h>
#include <math.h>
// ESP32 SDK includes.
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include <esp_mac.h>		// Needed for esp_read_mac(..)

// Custom includes.
#include "commands.h"
#include "serial_io.h"

// Function prototypes
int hex_to_dec(const char* hex);
int bin_to_dec(const char* bin);
int oct_to_dec(const char* oct);

enum CommandToken {
  COMMAND_UNKNOWN,
  COMMAND_MAC,
  COMMAND_ID,
  COMMAND_STATUS,
  COMMAND_DEC
};

typedef struct Command {
  enum CommandToken key;
  char command[MSG_BUFFER_LENGTH];
  char argument[MSG_BUFFER_LENGTH];
} Command;

void init_Command(Command * cmd) {
  if (!cmd) { return; }

  cmd->key = COMMAND_UNKNOWN;
  memset(cmd->command, 0, MSG_BUFFER_LENGTH);
  memset(cmd->argument, 0, MSG_BUFFER_LENGTH);
}


// Helper function forward declarations.
int parse_input(const char* in_msg, Command* out_cmd);
int process_cmd_id(const Command* in_cmd, char* out_msg);
int process_cmd_mac(const Command* in_cmd, char* out_msg);
int process_cmd_status(const Command* in_cmd, char* out_msg);
int process_cmd_dec(const Command* in_cmd, char* out_msg);

void mac_to_string(const uint8_t* mac, char* out);

// Implement a naive command dispatcher.  Better mechanism would be to
//	standardize definitions of commands (command token, number of args, etc.) and
//	present an interface to define/register commands during initialization.
int process_command(const char* in_msg, char* out_msg) {
  // Pre-condition: input and output char buffers must not be NULL.
  if (!in_msg || !out_msg) { return -1; } 

  // Keep an internal buffer for composing output string.
  char str_buffer[MSG_BUFFER_LENGTH];
  memset(str_buffer, 0, MSG_BUFFER_LENGTH);

  Command cmd;
  init_Command(&cmd);
  parse_input(in_msg, &cmd);
  int command_result = 0;

  switch(cmd.key) {
  case COMMAND_MAC: 
    command_result = process_cmd_mac(&cmd, str_buffer);
    break;

  case COMMAND_ID:
    command_result = process_cmd_id(&cmd, str_buffer);
    break;

  case COMMAND_STATUS:
    command_result = process_cmd_status(&cmd, str_buffer);
    break;

  case COMMAND_DEC:
    command_result = process_cmd_dec(&cmd, str_buffer);
    break;

  default:
    command_result = 1;	// Command error.
  }

  if (command_result) { return command_result; }

  // For safety, overwrite a null terminator onto our transient buffer,
  //	regardless of whether one was there or not previously.
  str_buffer[MSG_BUFFER_LENGTH - 1] = '\0';
  strncpy(out_msg, str_buffer, MSG_BUFFER_LENGTH);
  return 0;
}

int parse_input(const char* in_msg, Command* out_cmd) {
  // Precondition: in_msg and out_cmd may not be NULL.
  if (!in_msg || !out_cmd) { return -1; }

  const size_t len = strlen(in_msg);
  if (len >= MSG_BUFFER_LENGTH) {
    // Take an out here -- if somehow the input message is longer than the
    //	input message buffer then something has run amok.
    return -2;
  }

  // Zero the Command structure up front.
  init_Command(out_cmd);

  int at_src = -1;
  int at_dst = 0;
  int has_split = 0;
  char* target = out_cmd->command;
  while (++at_src < len) {
    switch(in_msg[at_src]) {
    case ' ':
    case '\t':
      if (!has_split) {
	has_split = 1;
	target = out_cmd->argument;
	at_dst = 0;
	break;
      }
      // Note intentional fallthrough here.  If whitespace is not being used to
      //	split command and argument, then it should be included in whatever
      //	target we're building.

    default:
      // If we are still filling the command token, force characters to lower-case
      //	to implement case insensitivity.
      target[at_dst++] = (has_split ? in_msg[at_src] : (char)tolower(in_msg[at_src]));
    }
  }
	
  if (!strcmp(out_cmd->command, "mac")) 			{ out_cmd->key = COMMAND_MAC; }
  else if (!strcmp(out_cmd->command, "id")) 		{ out_cmd->key = COMMAND_ID; }
  else if (!strcmp(out_cmd->command, "status")) 	{ out_cmd->key = COMMAND_STATUS; }
  else if (!strcmp(out_cmd->command, "dec")) 		{ out_cmd->key = COMMAND_DEC; }

  return 0;
}

int process_cmd_mac(const Command* in_cmd, char* out_msg) {
  uint8_t local_mac[6];
  const char* prefix = "MAC ";
  esp_read_mac(local_mac, ESP_MAC_WIFI_STA);
  strcpy(out_msg, prefix);
  mac_to_string(local_mac, out_msg + strlen(prefix));
  return 0;
}

void mac_to_string(const uint8_t* mac, char* out) {
  // Pre-condition: mac bytes and output string must not be NULL.
  if (!mac || !out) { return; }

  const char* hexmap = "0123456789ABCDEF";
  char sep = ':';

  int at = 0;
  for (int i = 0; i < 6; ++i) {
    int lo = (mac[i] & 0x0F);
    int hi = ((mac[i] & 0xF0) >> 4);
    out[at++] = hexmap[hi];
    out[at++] = hexmap[lo];
    if (i < 5) {
      out[at++] = sep;
    }
  }
}

int process_cmd_id(const Command* in_cmd, char* out_msg) {
  strcpy(out_msg, "gbt6@hi.is");
  return 0;
}

int process_cmd_status(const Command* in_cmd, char* out_msg) {
    int64_t time_in_microseconds = esp_timer_get_time();
    double time_in_seconds = time_in_microseconds / 1000000.0;
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    int cores = chip_info.cores;
    int mem = esp_get_free_heap_size();
    
    snprintf(out_msg, 100, "SYSTEM UPTIME: %.2lf S\nAVAILABLE CORES: %d\nAVAILABLE HEAP MEMORY: %d", time_in_seconds, cores, mem);
    
    return 0;
}

int process_cmd_dec(const Command* in_cmd, char* out_msg) {
  int number;
  char* argument = in_cmd->argument;
  if (strncmp(argument, "0b", 2) == 0) {
    number = bin_to_dec(argument + 2);
  } else if (strncmp(argument, "0x", 2) == 0) {
    number = hex_to_dec(argument + 2);
  } else if (argument[0] == '0') {
    number = oct_to_dec(argument + 1);
  } else {
    number = atoi(argument);
  }

  if (number == -1 || number > (1 << 16) - 1) {
    strcpy(out_msg, "ARGUMENT ERROR");
  } else {
    sprintf(out_msg, "%d", number);
  }

  return 0;
}

int hex_to_dec(const char* hex) {
  int len = strlen(hex);
  int dec_val = 0;
  int base = 1;

  for (int i = len - 1; i >= 0; i--) {
    int digit;
    if (hex[i] >= '0' && hex[i] <= '9') {
      digit = hex[i] - '0';
    } else if (hex[i] >= 'A' && hex[i] <= 'F') {
      digit = hex[i] - 'A' + 10;
    } else if (hex[i] >= 'a' && hex[i] <= 'f') {
      digit = hex[i] - 'a' + 10;
    } else {
      return -1;  // Invalid character
    }
    dec_val += digit * base;
    base *= 16;
  }

  return dec_val;
}

int oct_to_dec(const char* oct) {
  int len = strlen(oct);
  int dec_val = 0;
  int base = 1;

  for (int i = len - 1; i >= 0; i--) {
    if (oct[i] < '0' || oct[i] > '7') {
      return -1;  // Invalid character
    }
    int digit = oct[i] - '0';
    dec_val += digit * base;
    base *= 8;
  }

  return dec_val;
}
int bin_to_dec(const char* bin) {
  int len = strlen(bin);
  int dec_val = 0;
  int base = 1;

  for (int i = len - 1; i >= 0; i--) {
    if (bin[i] < '0' || bin[i] > '1') {
      return -1;  // Invalid character
    }
    int digit = bin[i] - '0';
    dec_val += digit * base;
    base *= 2;
  }

  return dec_val;
}
