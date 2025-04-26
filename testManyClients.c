#include "networks.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <ctype.h>
#include "sendPDU.h"
// for i = 0 to 300
// Connect to the server (this gets you a new socket)
// sprintf(handleName, “test%d”, i) // creates a new handle name
// call your function to registera new handleName with the server using the new socket
// Go back to top of for loop
int myHandleLength;
void sendInitialPacket(int socketNum, char * myHandle);

int main(int argc, char *argv[])
{
    char handleName[101] = {0};
    int socketNum = 0;
    for (int i = 0; i < 259; i++){
        socketNum = tcpClientSetup("::1", "50000", 1);
		sprintf(handleName, "test%d", i);
        myHandleLength = strlen(handleName);
        sendInitialPacket(socketNum, handleName);
    }
    while (1){

    }
}

void sendInitialPacket(int socketNum, char * myHandle)
{
	int packetSize = 1 + 1 + 100;
	uint8_t packet[packetSize];
	packet[0] = 1;
	packet[1] = myHandleLength;
	memcpy(&packet[2], myHandle, myHandleLength);
	sendPDU(socketNum, packet, packetSize);
}