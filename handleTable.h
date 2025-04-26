#include <stdbool.h>
#include "handleConstants.h"

#define STARTING_HANDLE_TABLE_SIZE 10

#define SUCCESSFUL_LOOKUP 0
#define FAILED_LOOKUP -1
#define ADD_HANDLE_SUCCESS 0
#define ADD_HANDLE_FAILURE -1

struct handleTable
{
    char handleName[MAX_HANDLE_SIZE + 1];
    int socketNum;
    bool valid;
};

char *getNextHandle();
int getNumActiveHandles();
int lookupByHandle(char *handleName, int *socketNum);
int lookupBySocket(int socketNum, char *handleName);
void removeBySocket(int socketNum);
void growTable();
int addHandle(int socketNum, char *handleName, int handleNameLen);
void setupHandleTable();