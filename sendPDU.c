#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "safeUtil.h"
#include "sendPDU.h"

#define PDU_HEADER_SIZE 2

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData)
{
    uint16_t lengthOfPDU = lengthOfData + PDU_HEADER_SIZE;
    uint8_t PDU_buff[lengthOfPDU];
    uint16_t lengthOfPDU_n = htons(lengthOfPDU);
    memcpy(PDU_buff, &lengthOfPDU_n, sizeof(uint16_t));
    memcpy(PDU_buff + sizeof(uint16_t), dataBuffer, lengthOfData);

    // don't need to check return value because safeSend does that
    safeSend(clientSocket, (uint8_t *)PDU_buff, lengthOfPDU, 0);

    return lengthOfData;
}