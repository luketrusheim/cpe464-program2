#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "handleUtil.h"
#include "handleConstants.h"

// stores the handle length and the handle (no null) in the given PDU and returns offset to next handle length
int insertHandle(char *handleName, uint8_t *pdu, int startingOffset)
{
    uint8_t handleLength = strlen(handleName);
    pdu[startingOffset] = handleLength;
    memcpy(pdu + startingOffset + HANDLE_LENGTH_SIZE, handleName, handleLength);
    return startingOffset + HANDLE_LENGTH_SIZE + handleLength;
}

// stores handle in handleName with null pointer and returns offset to next handle length
// if handleName points to NULL, then insertHandle will throw away handle but still return offset to next handle
int getHandle(char *handleName, uint8_t *pdu, int startingOffset)
{
    uint8_t handleLength = *(pdu + startingOffset);
    if (handleName != NULL) {
        memcpy(handleName, pdu + startingOffset + HANDLE_LENGTH_SIZE, handleLength);
        *(handleName + handleLength) = '\0';
    }
    return startingOffset + HANDLE_LENGTH_SIZE + handleLength;
}