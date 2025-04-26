#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "handleUtil.h"
#include "handleConstants.h"
#include "commandHelpers.h"
#include "flags.h"

int parseMessageCommand(char *command, char *receivingHandle, char *message)
{
    char *handle;

    handle = strtok(command, " ");
    if (handle != NULL)
        memcpy(receivingHandle, handle, MAX_HANDLE_SIZE + 1);
    else
        return MSG_PARSE_FAILURE;

    // was told we won't have input very near 1400 so while this should technically be 1400 bytes minus command stuff minus handle stuff, this should do for now
    memcpy(message, handle + strlen(handle) + 1, 1400);

    return MSG_PARSE_SUCCESS;
}

int parseMulticastCommand(char *command, char receivingHandles[][MAX_HANDLE_SIZE + 1], int *numHandles, char *message)
{
    char *numberOfReceivers_s;
    int numberOfReceivers_i;
    char *handle;

    char *token;
    token = strtok(command, " ");
    if (token == NULL)
        return MSG_PARSE_FAILURE;
    numberOfReceivers_s = token;
    numberOfReceivers_i = atoi(numberOfReceivers_s);
    if (numberOfReceivers_i < 2 || numberOfReceivers_i > 9)
        return MSG_INVALID_NUM_HANDLES;
    *numHandles = numberOfReceivers_i;

    for (int i = 0; i < numberOfReceivers_i; i++)
    {
        token = strtok(NULL, " ");
        handle = token;
        if (handle != NULL)
            memcpy(receivingHandles[i], handle, MAX_HANDLE_SIZE + 1);
        else
            return MSG_NOT_ENOUGH_HANDLES;
    }

    // was told we won't have input very near 1400 so while this should technically be 1400 bytes minus command stuff minus handle stuff, this should do for now
    memcpy(message, handle + strlen(handle) + 1, 1400);
    return MSG_PARSE_SUCCESS;
}

int createMulticastHeader(uint8_t *header, char *sendingHandle, char receivingHandles[][MAX_HANDLE_SIZE + 1], int numHandles)
{
    int offset = 0;
    int lenOfAllHandles = 0;
    header[0] = FLAG_MULTICAST;
    offset += FLAG_SIZE;

    offset = insertHandle(sendingHandle, header, offset);

    header[offset] = numHandles;
    offset += 1;

    for (int i = 0; i < numHandles; i++)
    {
        offset = insertHandle(receivingHandles[i], header, offset);
        lenOfAllHandles += strlen(receivingHandles[i]);
    }

    return FLAG_SIZE + HANDLE_LENGTH_SIZE + strlen(sendingHandle) + 1 + (HANDLE_LENGTH_SIZE * numHandles) + lenOfAllHandles;
}

int createMessageHeader(uint8_t *pdu, char *sendingHandle, char *receivingHandle)
{
    int offset = 0;
    pdu[0] = FLAG_MESSAGE;
    offset += FLAG_SIZE;

    offset = insertHandle(sendingHandle, pdu, offset);

    pdu[offset] = 1;
    offset += 1;

    offset = insertHandle(receivingHandle, pdu, offset);

    return FLAG_SIZE + HANDLE_LENGTH_SIZE + strlen(sendingHandle) + 1 + HANDLE_LENGTH_SIZE + strlen(receivingHandle);
}

int createBroadcastHeader(uint8_t *header, char *sendingHandle)
{
    int offset = 0;
    header[0] = FLAG_BROADCAST;
    offset += FLAG_SIZE;

    insertHandle(sendingHandle, header, offset);

    return FLAG_SIZE + HANDLE_LENGTH_SIZE + strlen(sendingHandle);
}