#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include "safeUtil.h"
#include "recvPDU.h"

int recvPDU(int socketNumber, uint8_t * dataBuffer, int bufferSize) {
    uint16_t lengthOfPDU_n = 0;
    uint16_t lengthOfPDU_h = 0;
    // uint8_t PDU_buff[bufferSize];
    int bytesReceived = 0;

    bytesReceived = safeRecv(socketNumber, (uint8_t *) &lengthOfPDU_n, sizeof(uint16_t), MSG_WAITALL);
    if (bytesReceived == 0) {
        printf("recvPDU: connection closed\n");
        return 0; // connection closed
    }

    lengthOfPDU_h = ntohs(lengthOfPDU_n);

    if (lengthOfPDU_h > bufferSize) {
        perror("recvPDU: buffer too small");
        exit(-1);
    }

    bytesReceived = safeRecv(socketNumber, dataBuffer, lengthOfPDU_h - 2, MSG_WAITALL);
    if (bytesReceived == 0) {
        return 0; // connection closed
    }

    // // copy the data to the provided buffer
    // memcpy(dataBuffer, PDU_buff, lengthOfPDU);

    return bytesReceived;
}