#include "handleConstants.h"

#define MAX_MSG_SIZE 199 // does not include null pointer
#define MESSAGE_COMMAND 0
#define MULTICAST_COMMAND 1

#define MSG_NOT_ENOUGH_HANDLES -3
#define MSG_INVALID_NUM_HANDLES -2
#define MSG_PARSE_FAILURE -1
#define MSG_PARSE_SUCCESS 0

int createBroadcastHeader(uint8_t * header, char *sendingHandle);

int createMulticastPDU(uint8_t * pdu, char *sendingHandle, char receivingHandles[][MAX_HANDLE_SIZE + 1], int numHandles, char *message);
int parseMulticastCommand(char *command, char receivingHandles[][MAX_HANDLE_SIZE + 1], int *numHandles, char *message);

int parseMessageCommand(char *command, char *receivingHandle, char *message);
int createMessagePDU(uint8_t * pdu, char * sendingHandle, char *receivingHandle, char *message);