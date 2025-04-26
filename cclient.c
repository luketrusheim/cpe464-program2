/******************************************************************************
 * myClient.c
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
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <ctype.h>

#include "flags.h"
#include "commandHelpers.h"

#include "handleTable.h"
#include "handleUtil.h"
#include "handleConstants.h"

#include "networks.h"
#include "safeUtil.h"
#include "sendPDU.h"
#include "recvPDU.h"
#include "pollLib.h"

#define MAXBUF 1400
#define DEBUG_FLAG 1

void clientControl(int socketNum);
void initializeClient(int socketNum, char *handle);
void sendInitialPacket(int socketNum);
uint8_t getHandleStatus(int socketNum);

void printHandleInfo(uint8_t * pdu);

void processMessageCommand(int socketNum, char *command);
void processMulticastCommand(int socketNum, char *command);
void processBroadcastCommand(int socketNum, char * command);

void processIncomingPacket(int socketNum);
void readMessage(uint8_t * buffer, int messageType);
void sendMessageWithHeader(int socketNum, uint8_t * header, int headerLen, char * completeMessage);
void parseHandleDoesntExistPacket(uint8_t * pdu);

void processStdin(int socketNum);
int readFromStdin(char *buffer);

void checkArgs(int argc, char *argv[]);

char myHandle[MAX_HANDLE_SIZE + 1];
uint8_t myHandleLength;

int main(int argc, char *argv[])
{
	int socketNum = 0; // socket descriptor

	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	initializeClient(socketNum, argv[1]);

	clientControl(socketNum);

	close(socketNum);

	return 0;
}

void initializeClient(int socketNum, char *handle)
{
	myHandleLength = strlen(handle);

	if (myHandleLength > 100)
	{
		printf("Invalid handle, handle longer than 100 characters: %s", handle);
		exit(-1);
	}

	memcpy(myHandle, handle, myHandleLength); // store handle globally for use later

	sendInitialPacket(socketNum);

	uint8_t flag = getHandleStatus(socketNum);
	if (flag == FLAG_GOOD_HANDLE)
		return;
	else if (flag == FLAG_HANDLE_EXISTS)
		printf("Handle already in use: %s\n", handle);
	else
		printf("Unexpected flag: %u\n", flag);
	exit(-1);
}

// tell server our desired handle name
void sendInitialPacket(int socketNum)
{
	int packetSize = FLAG_SIZE + HANDLE_LENGTH_SIZE + MAX_HANDLE_SIZE;
	uint8_t packet[packetSize];
	packet[0] = FLAG_INITAL_PACKET;
	packet[1] = myHandleLength;
	memcpy(&packet[2], myHandle, myHandleLength);
	sendPDU(socketNum, packet, packetSize);
}

// waits for response from server telling us if handle is good or bad
uint8_t getHandleStatus(int socketNum)
{
	uint8_t flag;
	int recvBytes = 0;
	recvBytes = recvPDU(socketNum, &flag, MAXBUF); // will block until flag is recieved
	if (recvBytes <= 0)
	{
		printf("Server terminated while waiting for handle initialization flag\n");
		exit(0);
	}
	return flag;
}

// wait for input from stdin or message from server
void clientControl(int socketNum)
{
	int readyFileDesc = 0;
	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(socketNum);

	while (1)
	{
		printf("$: ");
		fflush(stdout);
		readyFileDesc = pollCall(POLL_WAIT_FOREVER);
		if (readyFileDesc == STDIN_FILENO) processStdin(socketNum);
		else processIncomingPacket(readyFileDesc);
	}
}

void processIncomingPacket(int socketNum)
{
	uint8_t buffer[MAXBUF] = {0};
	int recvBytes = 0;
	recvBytes = recvPDU(socketNum, buffer, MAXBUF);
	if (recvBytes > 0) {
		// printf("Socket %d | Bytes recv: %d | Message: %s\n", socketNum, recvBytes, buffer);
		uint8_t flag = buffer[0];
		// printf("Flag on incoming packet: %u\n", flag);

		switch (flag) {
			case FLAG_MESSAGE:
				readMessage(buffer + FLAG_SIZE, FLAG_MESSAGE);
				break;
		
			case FLAG_HANDLE_DOESNT_EXIST:
				parseHandleDoesntExistPacket(buffer + FLAG_SIZE);
				// forwardMessage(dataBuffer + 1, packetLen - 1);
				break;
		
			case FLAG_MULTICAST:
				readMessage(buffer + FLAG_SIZE, FLAG_MULTICAST);
				break;

			case FLAG_HANDLE_LIST_LENGTH:
				{
				uint32_t numActiveHandles_n = 0;
				memcpy(&numActiveHandles_n, buffer + FLAG_SIZE, 4);
				printf("\nNumber of clients: %d\n", ntohl(numActiveHandles_n));
				}
				break;

			case FLAG_HANDLE_INFO:
				printHandleInfo(buffer + FLAG_SIZE);
				processIncomingPacket(socketNum);
				break;
			
			case FLAG_HANDLE_LIST_REQUEST_COMPLETE:
				break;
			
			case FLAG_BROADCAST:
				readMessage(buffer + FLAG_SIZE, FLAG_BROADCAST);
				break;
		
			default:
				printf("flag unidentified\n");
				break;
			}
	} else {
		printf("Server terminated\n");
		exit(0);
	}
}

void printHandleInfo(uint8_t * pdu)
{
	char handleName[MAX_HANDLE_SIZE] = {0};
	getHandle(handleName, pdu, 0);
	printf("\t%s\n", handleName);
}

void parseHandleDoesntExistPacket(uint8_t * pdu) {
	char errorHandle[MAX_HANDLE_SIZE] = {0};
	int offset = 0;

	getHandle(errorHandle, pdu, offset);
	printf("\nClient with handle %s does not exist\n", errorHandle);
}

void readMessage(uint8_t * buffer, int messageType) {
	char sendingHandle[MAX_HANDLE_SIZE] = {0};
	char message[MAX_MSG_SIZE + 1] = {0};
	int offset = 0;
	int numHandles = 0;

	offset = getHandle(sendingHandle, buffer, offset);

	if (messageType != FLAG_BROADCAST) {
		numHandles = buffer[offset++];

		// skip past receiving handles
		for (int i = 0; i < numHandles; i++) {
			offset = getHandle(NULL, buffer, offset);
		}
	}
	
	memcpy(message, buffer + offset, MAX_MSG_SIZE + 1);

	printf("\n%s: %s\n", sendingHandle, message);
}

void processStdin(int socketNum)
{
	char command[MAXBUF] = {0}; // data buffer
	int commandLen = 0;

	commandLen = readFromStdin(command);
	printf("Read: %s | String len: %d (including null)\n", command, commandLen);

	if (command[0] != '%')
	{
		printf("Invalid command: command begins with %%\n");
	}
	else
	{
		switch (tolower(command[1]))
		{
		case 'm':
			processMessageCommand(socketNum, command + 2); // advance by two to skip the command indicator
			break;
		case 'b':
			processBroadcastCommand(socketNum, command + 2);
			break;
		case 'c':
			processMulticastCommand(socketNum, command + 2);
			break;
		case 'l':
			{
				uint8_t flag = FLAG_HANDLE_LIST_REQUEST;
				sendPDU(socketNum, &flag, FLAG_SIZE);
			}
			break;
		default:
			printf("Invalid command: %%%c is not a valid command\n", command[1]);
			break;
		}
	}
}

void processBroadcastCommand(int socketNum, char * command) {
	uint8_t header[FLAG_SIZE + HANDLE_LENGTH_SIZE + MAX_HANDLE_SIZE] = {0};
	int headerLen = 0;
	char completeMessage[MAXBUF] = {0};

	memcpy(completeMessage, command, 1400);

	headerLen = createBroadcastHeader(header, myHandle);
	printf("headerlen: %d\n", headerLen);
	sendMessageWithHeader(socketNum, header, headerLen, completeMessage);
}

void sendMessageWithHeader(int socketNum, uint8_t * header, int headerLen, char * completeMessage)
{
	uint8_t pdu[MAXBUF] = {0};
	char *messageChunk = completeMessage;

	do {
		int chunkSize = strlen(messageChunk);
		if (chunkSize > MAX_MSG_SIZE) {
			chunkSize = MAX_MSG_SIZE;
		}

		memcpy(pdu, header, headerLen);
		memcpy(pdu + headerLen, messageChunk, chunkSize);
		*(pdu + headerLen + chunkSize) = '\0';

		int pduLen = headerLen + chunkSize + 1;
		sendPDU(socketNum, pdu, pduLen);

		messageChunk += chunkSize;
	} while (*messageChunk != '\0');
}

void processMulticastCommand(int socketNum, char *command) {
	uint8_t pdu[1400] = {0};
	char receivingHandles[9][MAX_HANDLE_SIZE + 1] = {{0}};
	int numHandles = 0;
	char completeMessage[MAXBUF] = {0};

	int returnStatus = 0;
	returnStatus = parseMulticastCommand(command, receivingHandles, &numHandles, completeMessage);

	// for (int i = 0; i < 9; i++) {
	// 	printf("handle %d: %s\n", i, receivingHandles[i]);
	// 	printf("num handles: %d\n", numHandles);
	// }
	
	switch (returnStatus) {
		case (MSG_PARSE_SUCCESS):
			break;
		case (MSG_PARSE_FAILURE):
			printf("Invalid message command format\n");
			return;
		case (MSG_INVALID_NUM_HANDLES):
			printf("Invalid message command format: 1 < number of handles < 10\n");
			return;
		case (MSG_NOT_ENOUGH_HANDLES):
			printf("Invalid message command format: given number of handles did not match expected\n");
			return;
	}

	char *messageChunk = completeMessage;

	do {
		char message[MAX_MSG_SIZE + 1] = {0};

		int chunkSize = strlen(messageChunk);
		if (chunkSize > MAX_MSG_SIZE) {
			chunkSize = MAX_MSG_SIZE;
		}

		memcpy(message, messageChunk, chunkSize);
		message[chunkSize] = '\0';

		int pduLen = createMulticastPDU(pdu, myHandle, receivingHandles, numHandles, message);
		sendPDU(socketNum, pdu, pduLen);

		messageChunk += chunkSize;
	} while (*messageChunk != '\0');
}

void processMessageCommand(int socketNum, char *command)
{
	uint8_t pdu[1400] = {0};
	char receivingHandle[MAX_HANDLE_SIZE + 1] = {0};
	char completeMessage[MAXBUF] = {0};

	if (parseMessageCommand(command, receivingHandle, completeMessage) == MSG_PARSE_FAILURE)
	{
		printf("Invalid message command format\n");
		return;
	}

	char *messageChunk = completeMessage;

	do {
		char message[MAX_MSG_SIZE + 1] = {0};

		int chunkSize = strlen(messageChunk);
		if (chunkSize > MAX_MSG_SIZE) {
			chunkSize = MAX_MSG_SIZE;
		}

		memcpy(message, messageChunk, chunkSize);
		message[chunkSize] = '\0';

		int pduLen = createMessagePDU(pdu, myHandle, receivingHandle, message);
		sendPDU(socketNum, pdu, pduLen);

		messageChunk += chunkSize;
	} while (*messageChunk != '\0');
}

int readFromStdin(char *buffer)
{
	char aChar = 0;
	int inputLen = 0;

	// Important you don't input more characters than you have space
	buffer[0] = '\0';
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}

	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;

	return inputLen;
}

void checkArgs(int argc, char *argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle server-name server-port\n", argv[0]);
		exit(1);
	}
}
