/******************************************************************************
 * myServer.c
 *
 * Writen by Prof. Smith, updated Jan 2023
 * Use at your own risk.
 *
 *****************************************************************************/

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
#include <netdb.h>
#include <stdint.h>

#include "flags.h"
#include "handleUtil.h"
#include "handleTable.h"
#include "networks.h"
#include "safeUtil.h"
#include "recvPDU.h"
#include "sendPDU.h"
#include "pollLib.h"

#define MAXBUF 1400
#define DEBUG_FLAG 1

void processNewHandle(int socketNum, uint8_t * handlePacket, int packetLen);
void processIncomingPacket(int clientSocket, uint8_t * dataBuffer, int packetLen);
void addNewSocket(int mainServerSocket);
void processClient(int polledSocket);
void serverControl(int mainServerSocket);

void sendHandleList(int socketNum);
void processHandleListRequest(int socketNum);
void processMessage(uint8_t * pdu, int pduLen, int sendingSocket);
void processBroadcast(uint8_t * pdu, int pduLen, int sendingSocket);
void sendHandleDoesntExistPacket(int socketNum, char * errorHandle);

// returns packet length and stores packet in dataBuffer
int recvFromClient(int clientSocket, uint8_t * dataBuffer);

int checkArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	int mainServerSocket = 0; // socket descriptor for the server socket
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);

	// create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	serverControl(mainServerSocket);

	/* close the socket */
	close(mainServerSocket);

	return 0;
}

void serverControl(int mainServerSocket)
{
	int readySocket = 0;
	
	setupHandleTable();
	setupPollSet();
	addToPollSet(mainServerSocket);
	
	while (1) {
		readySocket = pollCall(POLL_WAIT_FOREVER);
		if (readySocket == mainServerSocket) {
			addNewSocket(readySocket);
		} else{
			processClient(readySocket);
		}
	}
}

void addNewSocket(int mainServerSocket) {
	int clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
	printf("New client connected | Socket: %d\n", clientSocket);
}

void processClient(int clientSocket) {
	uint8_t dataBuffer[MAXBUF] = {0};
	int packetLen = recvFromClient(clientSocket, dataBuffer);
	
	if (packetLen != 0) {
		processIncomingPacket(clientSocket, dataBuffer, packetLen);
	} else {
		removeFromPollSet(clientSocket);
		removeBySocket(clientSocket);
		printf("Socket: %d | Connection closed by other side\n", clientSocket);
	}
}

int recvFromClient(int clientSocket, uint8_t * dataBuffer)
{
	int packetLen = 0;

	// now get the data from the client_socket
	if ((packetLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		//this should never happen
		perror("recv from client negative return");
		exit(-1);
	}

	if (packetLen > 0) printf("Socket %d | Bytes recv: %d | Message: %s\n", clientSocket, packetLen, dataBuffer);

	return packetLen;
}

void processIncomingPacket(int socketNum, uint8_t * dataBuffer, int packetLen) {
	uint8_t flag = dataBuffer[0];
	printf("Flag on incoming packet: %u\n", flag);

	switch (flag) {
		case FLAG_INITAL_PACKET:
			processNewHandle(socketNum, dataBuffer + 1, packetLen - 1);
			break;
	
		case FLAG_MESSAGE: {
			processMessage(dataBuffer, packetLen, socketNum);
			break;
		}
		case FLAG_MULTICAST:
			processMessage(dataBuffer, packetLen, socketNum);
			break;
		
		case FLAG_BROADCAST:
			processBroadcast(dataBuffer, packetLen, socketNum);
			break;

		case FLAG_HANDLE_LIST_REQUEST:
			processHandleListRequest(socketNum);
			break;

		default:
			printf("flag unidentified\n");
			break;
		}
}

void processBroadcast(uint8_t * pdu, int pduLen, int sendingSocket)
{
	char *handle = getNextHandle();
	int receivingSocket = 0; 

	while (handle != NULL) {
		lookupByHandle(handle, &receivingSocket);
		handle = getNextHandle();
		if (receivingSocket == sendingSocket) continue;
		sendPDU(receivingSocket, pdu, pduLen);
		
	} 
}

void processHandleListRequest(int socketNum) {
	uint32_t numActiveHandles_n = htonl(getNumActiveHandles());
	uint8_t pdu[FLAG_SIZE + 4] = {0};

	pdu[0] = FLAG_HANDLE_LIST_LENGTH;
	memcpy(pdu + FLAG_SIZE, &numActiveHandles_n, 4);
	sendPDU(socketNum, pdu, FLAG_SIZE + 4);

	sendHandleList(socketNum);

	uint8_t flag = FLAG_HANDLE_LIST_REQUEST_COMPLETE;
	sendPDU(socketNum, &flag, FLAG_SIZE);
}

void sendHandleList(int socketNum) {
	char *handle = getNextHandle();

	while (handle != NULL) {
		uint8_t pdu[FLAG_SIZE + HANDLE_LENGTH_SIZE + MAX_HANDLE_SIZE] = {0};
		uint8_t handleLen = strlen(handle);
		pdu[0] = FLAG_HANDLE_INFO;
		pdu[1] = handleLen;
		memcpy(pdu + FLAG_SIZE + HANDLE_LENGTH_SIZE, handle, handleLen);
		sendPDU(socketNum, pdu, FLAG_SIZE + HANDLE_LENGTH_SIZE + handleLen);

		handle = getNextHandle();
	} 
}

void sendHandleDoesntExistPacket(int socketNum, char * errorHandle) {
	uint8_t pdu[MAXBUF] = {0};
	pdu[0] = FLAG_HANDLE_DOESNT_EXIST;
	int errorHandleLen = strlen(errorHandle);
	int offset = FLAG_SIZE;
	offset = insertHandle(errorHandle, pdu, offset);
	sendPDU(socketNum, pdu, FLAG_SIZE + HANDLE_LENGTH_SIZE + errorHandleLen);
}

int getHandles(uint8_t * pdu, char handles[9][MAX_HANDLE_SIZE + 1]) {
	int offset = FLAG_SIZE;
	int numHandles = 0;

	offset = getHandle(NULL, pdu, offset); // don't care about the sending handle
	numHandles = pdu[offset++];

	for (int i = 0; i < numHandles; i++) {
		offset = getHandle(handles[i], pdu, offset);
	}

	return numHandles;
}

void processMessage(uint8_t * pdu, int pduLen, int sendingSocket){
	int receivingSocket = 0;
	char receivingHandles[9][MAX_HANDLE_SIZE + 1] = {0};
	int numHandles = 0;

	numHandles = getHandles(pdu, receivingHandles);

	for (int i = 0; i < numHandles; i++) {
		if (lookupByHandle(receivingHandles[i], &receivingSocket) == FAILED_LOOKUP) {
			printf("Failed lookup for handle: %s\n", receivingHandles[i]);
			sendHandleDoesntExistPacket(sendingSocket, receivingHandles[i]);
		} else {
			sendPDU(receivingSocket, pdu, pduLen);
		}
	}
}

void processNewHandle(int socketNum, uint8_t * handlePacket, int packetLen) {
	char handleName[MAX_HANDLE_SIZE + 1] = {0};
	getHandle(handleName, handlePacket, 0);
	printf("Handle name: %s\n", handleName);

	uint8_t returnFlag;
	if (addHandle(socketNum, handleName, strlen(handleName)) == ADD_HANDLE_SUCCESS) returnFlag = FLAG_GOOD_HANDLE;
	else returnFlag = FLAG_HANDLE_EXISTS;

	sendPDU(socketNum, &returnFlag, 1);
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}

	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}

	return portNumber;
}
