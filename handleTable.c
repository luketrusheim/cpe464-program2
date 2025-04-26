#include <stdio.h>
#include <string.h>
#include "safeUtil.h"
#include "handleTable.h"

static struct handleTable *myHandleTable;
int currentTableSize;
int activeHandles;
static int getNextHandleIndex;
bool handleIsUnique(char *handleName);

void setupHandleTable()
{
    currentTableSize = STARTING_HANDLE_TABLE_SIZE;
    myHandleTable = (struct handleTable *)sCalloc(STARTING_HANDLE_TABLE_SIZE, sizeof(struct handleTable));
    activeHandles = 0;
    getNextHandleIndex = 0;
}

// handleNameLen does not include null terminator
int addHandle(int socketNum, char *handleName, int handleNameLen)
{
    if (!handleIsUnique(handleName))
    {
        printf("Handle is not unique\n");
        return ADD_HANDLE_FAILURE;
    }

    while (socketNum >= currentTableSize)
    {
        growTable();
    }

    struct handleTable *newHandle = &(myHandleTable[socketNum]);
    memcpy(newHandle->handleName, handleName, handleNameLen + 1);
    newHandle->socketNum = socketNum;
    newHandle->valid = true;

    activeHandles++;
    return ADD_HANDLE_SUCCESS;
}

void growTable()
{
    // double size of table
    myHandleTable = srealloc(myHandleTable, sizeof(struct handleTable) * (currentTableSize * 2));

    // invalidate new handles
    for (int i = currentTableSize; i < (currentTableSize * 2); i++)
    {
        myHandleTable[i].valid = false;
    }

    currentTableSize *= 2;
}

int lookupByHandle(char *handleName, int *socketNum)
{
    for (int i = 0; i < currentTableSize; i++)
    {
        if (myHandleTable[i].valid)
        {
            if (!strcmp(handleName, myHandleTable[i].handleName))
            {
                *socketNum = i;
                return SUCCESSFUL_LOOKUP;
            }
        }
    }
    return FAILED_LOOKUP;
}

int lookupBySocket(int socketNum, char *handleName)
{
    if (currentTableSize < socketNum + 1)
    {
        printf("Socket lookup (%d) is out of bounds\n", socketNum);
        return FAILED_LOOKUP;
    }

    if (myHandleTable[socketNum].valid)
    {
        handleName = myHandleTable[socketNum].handleName;
        return SUCCESSFUL_LOOKUP;
    }
    else
        return FAILED_LOOKUP;
}

void removeBySocket(int socketNum)
{
    myHandleTable[socketNum].valid = false;
    printf("Removing handle \"%s\" with socket num %d\n", (myHandleTable[socketNum].handleName), socketNum);
    activeHandles--;
}

bool handleIsUnique(char *handleName)
{
    for (int i = 0; i < currentTableSize; i++)
    {
        if (myHandleTable[i].valid)
        {
            if (!strcmp(handleName, myHandleTable[i].handleName))
                return false;
        }
    }
    return true;
}

int getNumActiveHandles()
{
    return activeHandles;
}


char *getNextHandle()
{
    for (; getNextHandleIndex < currentTableSize; getNextHandleIndex++)
    {
        if (myHandleTable[getNextHandleIndex].valid) return myHandleTable[getNextHandleIndex++].handleName;
    }

    getNextHandleIndex = 0;
    return NULL;
}


