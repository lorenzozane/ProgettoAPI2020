//#define LOCAL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum commands {
    c_command = 1, //change
    d_command = 2, //delete
    p_command = 3, //print
    u_command = 4, //undo
    r_command = 5, //redo
    q_command = 6 //quit
};

typedef struct node {
    char commandType;
    int command[2];
    int32_t *changedLines;
    int finalOutputStateDimension;
    struct node *next;
} NODE;


#ifdef LOCAL
FILE *fp;
FILE *fout;
#endif

enum commands currentCommand;
NODE *undoableListFirst = NULL;
NODE *redoableListFirst = NULL;
int undoNodeCounter = 0;
int redoNodeCounter = 0;
int redoToDo = 0;
int undoToDo = 0;
int undoRedoable = 0;


char **inputText;
int inputTextDimension = 0;
int inputTextMemoryDimension = 0;
int32_t *finalOutput;
int finalOutputDimension = 0;
//char ignored[11];
char ignore;

enum commands parser(char *lineToParse) {
    char commandChar = lineToParse[strlen(lineToParse) - 2];
    enum commands command = q_command;

    if (commandChar == 'c')
        command = c_command;
    else if (commandChar == 'd')
        command = d_command;
    else if (commandChar == 'p')
        command = p_command;
    else if (commandChar == 'u')
        command = u_command;
    else if (commandChar == 'r')
        command = r_command;
    else if (commandChar == 'q')
        command = q_command;

    return command;
}

void readInput(int index, bool needRealloc, NODE *nodeToInsert, int saveIndex, bool readFromRedoable) {
    char readLine[1025];

    if (needRealloc) {
        nodeToInsert->changedLines[saveIndex] = -1;
    } else {
        if (finalOutput[index] == -1)
            nodeToInsert->changedLines[saveIndex] = -1;
        else
            nodeToInsert->changedLines[saveIndex] = finalOutput[index];
    }

    if (!readFromRedoable) {
#ifdef LOCAL
        if (fgets(readLine, 1024, fp) != NULL) {
            inputText[inputTextDimension] = malloc((strlen(readLine) + 1) * sizeof(char));
            strcpy(inputText[inputTextDimension], readLine);

            finalOutput[index] = inputTextDimension;

            inputTextDimension++;
        }
#else
        if (fgets(readLine, 1024, stdin) != NULL) {
            inputText[inputTextDimension] = malloc((strlen(readLine) + 1) * sizeof(char));
            strcpy(inputText[inputTextDimension], readLine);

            finalOutput[index] = inputTextDimension;

            inputTextDimension++;
        }
#endif
    } else {
        finalOutput[index] = redoableListFirst->changedLines[saveIndex];
    }
}

void printOutput(int index1, int index2) {
#ifdef LOCAL
        for (int i = index1; i < index2; i++) {
            if (!(i == -1 || i >= finalOutputDimension) && finalOutput[i] != -1) {
                fputs(inputText[finalOutput[i]], fout);
            } else {
                fputc('.', fout);
                fputc('\n', fout);
                }
        }
#else
    for (int i = index1; i < index2; i++) {
        if (!(i == -1 || i >= finalOutputDimension) && finalOutput[i] != -1) {
            fputs(inputText[finalOutput[i]], stdout);
        } else {
            fputc('.', stdout);
            fputc('\n', stdout);
        }
    }

#endif
}

void shiftElements(int initialShiftIndex, int finalShiftIndex) {
    int shiftDimension = finalShiftIndex - initialShiftIndex + 1;

    for (int i = 0; i < finalOutputDimension - shiftDimension; i++) {
        if (finalShiftIndex + i < finalOutputDimension) {
            if (finalOutput[finalShiftIndex + i] == -1) {
                finalOutput[initialShiftIndex - 1 + i] = -1;
            } else {
                finalOutput[initialShiftIndex - 1 + i] = finalOutput[finalShiftIndex + i];
            }
        } else
            break;
    }
}

NODE *createNewNodeIndex(char commandType, int index1, int index2, int changedLinesDimension) {
    NODE *newNode;
    newNode = malloc(sizeof(NODE));

    newNode->commandType = commandType;
    newNode->command[0] = index1;
    newNode->command[1] = index2;
    newNode->changedLines = malloc(changedLinesDimension * sizeof(int32_t));

    return newNode;
}

NODE *insertNode(NODE *nodeToInsert, NODE *listFirst) {
    if (listFirst == undoableListFirst)
        undoNodeCounter++;
    else if (listFirst == redoableListFirst)
        redoNodeCounter++;

    nodeToInsert->next = listFirst;

    return nodeToInsert;
}

void deleteNodeUndoable() {
    NODE *temp = undoableListFirst;
    temp = temp->next;

    if (undoableListFirst->commandType == 1) {
        free(undoableListFirst->changedLines);
        free(undoableListFirst);
    }

    undoableListFirst = temp;
    undoNodeCounter--;
}

void deleteNodeRedoable() {
    NODE *temp = redoableListFirst;
    temp = temp->next;

    free(redoableListFirst->changedLines);
    free(redoableListFirst);

    redoableListFirst = temp;
    redoNodeCounter--;
}

void undoChange() {
    int index1 = undoableListFirst->command[0];
    int index2 = undoableListFirst->command[1];

    if (index1 == 0 && index2 == 0) {
        NODE *nodeToAdd = createNewNodeIndex(undoableListFirst->commandType, undoableListFirst->command[0],
                                             undoableListFirst->command[1], (index2 - index1 + 1));
        redoableListFirst = insertNode(nodeToAdd, redoableListFirst);
        deleteNodeUndoable();
        return;
    }

    NODE *nodeToAdd = createNewNodeIndex(undoableListFirst->commandType, undoableListFirst->command[0],
                                         undoableListFirst->command[1], (index2 - index1 + 1));

    int indexToFree = 0;
    int j = index2 - 1;

    for (int i = index2 - index1 + 1; i > 0; i--, j--) {
        if (undoableListFirst->changedLines[i - 1] == -1) {
            nodeToAdd->changedLines[i - 1] = finalOutput[j];
            if ((i + index1 - 1) == finalOutputDimension || indexToFree > 0)
                indexToFree++;
        } else {
            if (indexToFree > 0) {
                finalOutput = realloc(finalOutput, (finalOutputDimension - indexToFree) * sizeof(int32_t));
                finalOutputDimension -= indexToFree;
                index2 -= indexToFree;
                indexToFree = 0;
            }

            nodeToAdd->changedLines[i - 1] = finalOutput[j];

            finalOutput[j] = undoableListFirst->changedLines[i - 1];
        }
    }

    if (indexToFree > 0) {
        finalOutput = realloc(finalOutput, (finalOutputDimension - indexToFree) * sizeof(int32_t));
        finalOutputDimension -= indexToFree;

        if (finalOutputDimension > 0) {
            int toFree = 0;

            while (finalOutput[finalOutputDimension - 1] == -1 && finalOutputDimension >= 1) {
                toFree++;
            }

            if (toFree > 0) {
                finalOutput = realloc(finalOutput, (finalOutputDimension - toFree) * sizeof(int32_t));
                finalOutputDimension -= toFree;
            }
        }
    }

    redoableListFirst = insertNode(nodeToAdd, redoableListFirst);
    deleteNodeUndoable();
}

void undoDelete() {
    int index1 = undoableListFirst->command[0];
    int index2 = undoableListFirst->command[1];

    if (index1 == 0 && index2 == 0) {
        NODE *nodeToAdd = createNewNodeIndex(undoableListFirst->commandType, undoableListFirst->command[0],
                                             undoableListFirst->command[1], (index2 - index1 + 1));
        redoableListFirst = insertNode(nodeToAdd, redoableListFirst);
        deleteNodeUndoable();
        return;
    }

    NODE *nodeToAdd = createNewNodeIndex(undoableListFirst->commandType, undoableListFirst->command[0],
                                         undoableListFirst->command[1], 0);

    if (undoableListFirst->changedLines != NULL) {
        free(finalOutput);
        finalOutput = undoableListFirst->changedLines;
        finalOutputDimension = undoableListFirst->finalOutputStateDimension;
    }

    redoableListFirst = insertNode(nodeToAdd, redoableListFirst);
    deleteNodeUndoable();
}

int commandHandler(char commandType, int index1, int index2, bool readFromRedoable) {
    enum commands command = (unsigned char) commandType;

    if (index1 == 0 && index2 >= 1 && command != 3)
        index1 = 1;

    //Change command
    if (command == 1) {
        if (index1 == 0 && index2 == 0) {
            NODE *nodeToAdd = createNewNodeIndex(commandType, index1, index2, 0);
            undoableListFirst = insertNode(nodeToAdd, undoableListFirst);
            return 0;
        }

        NODE *nodeToAdd = createNewNodeIndex(commandType, index1, index2, (index2 - index1 + 1));
        int lineToSaveCounter = 0;

        if (index1 <= finalOutputDimension) {
            if (index2 <= finalOutputDimension) {
                for (int i = index1 - 1; i < index2; i++) {
                    if (finalOutput[i] == -1)
                        readInput(i, true, nodeToAdd, lineToSaveCounter++, readFromRedoable);
                    else
                        readInput(i, false, nodeToAdd, lineToSaveCounter++, readFromRedoable);
                }
            } else {
                for (int i = index1 - 1; i < finalOutputDimension; i++) {
                    readInput(i, false, nodeToAdd, lineToSaveCounter++, readFromRedoable);
                }
                finalOutput = realloc(finalOutput, (index2) * sizeof(int32_t));
                for (int i = finalOutputDimension; i < index2; i++) {
                    finalOutput[i] = -1;
                    readInput(i, true, nodeToAdd, lineToSaveCounter++, readFromRedoable);
                }
                finalOutputDimension = index2;
            }
        } else {
            finalOutput = realloc(finalOutput, (index2) * sizeof(int32_t));
            if (index1 == finalOutputDimension + 1) {
                for (int i = index1 - 1; i < index2; i++) {
                    readInput(i, true, nodeToAdd, lineToSaveCounter++, readFromRedoable);
                }
            } else {
                for (int i = finalOutputDimension - 1; i < index1; i++) {
                    finalOutput[i] = -1;
                }
                for (int i = index1 - 1; i < index2; i++) {
                    readInput(i, true, nodeToAdd, lineToSaveCounter++, readFromRedoable);
                }
            }

            finalOutputDimension = index2;
        }

        undoableListFirst = insertNode(nodeToAdd, undoableListFirst);

        if (!readFromRedoable) {
#ifdef LOCAL
            do {
                ignore = (char)fgetc_unlocked(fp);
            } while (ignore != '\n');
#else
            do {
                ignore = (char) fgetc_unlocked(stdin);
            } while (ignore != '\n');
#endif
        }
    } else if (command == 2) {
        if (index1 == 0 && index2 == 0) {
            NODE *nodeToAdd = createNewNodeIndex(commandType, index1, index2, 0);
            undoableListFirst = insertNode(nodeToAdd, undoableListFirst);
            return 0;
        }

        NODE *nodeToAdd = createNewNodeIndex(commandType, index1, index2, finalOutputDimension);

        if (index1 <= finalOutputDimension && index2 <= finalOutputDimension) {
            if (finalOutput == NULL)
                nodeToAdd->changedLines = NULL;
            else {
                for (int i = 0; i < finalOutputDimension; i++) {
                    nodeToAdd->changedLines[i] = finalOutput[i];
                }
            }
            nodeToAdd->finalOutputStateDimension = finalOutputDimension;

            shiftElements(index1, index2);
            for (int i = finalOutputDimension; i > finalOutputDimension - (index2 - index1 + 1); i--) {
                finalOutput[i - 1] = -1;
            }
            finalOutput = realloc(finalOutput, (finalOutputDimension - (index2 - index1 + 1)) * sizeof(int32_t));
            finalOutputDimension = finalOutputDimension - (index2 - index1 + 1);
        } else if (index1 <= finalOutputDimension && index2 > finalOutputDimension) {
            if (finalOutput == NULL)
                nodeToAdd->changedLines = NULL;
            else {
                for (int i = 0; i < finalOutputDimension; i++) {
                    nodeToAdd->changedLines[i] = finalOutput[i];
                }
            }
            nodeToAdd->finalOutputStateDimension = finalOutputDimension;

            finalOutput = realloc(finalOutput,
                                  (finalOutputDimension - (finalOutputDimension - index1 + 1)) * sizeof(int32_t));
            finalOutputDimension = finalOutputDimension - (finalOutputDimension - index1 + 1);
        } else {
            if (finalOutput == NULL)
                nodeToAdd->changedLines = NULL;
            else {
                for (int i = 0; i < finalOutputDimension; i++) {
                    nodeToAdd->changedLines[i] = finalOutput[i];
                }
            }
            nodeToAdd->finalOutputStateDimension = finalOutputDimension;
        }

        undoableListFirst = insertNode(nodeToAdd, undoableListFirst);
    } else if (command == 3) {
        if (index1 == 0) {
#ifdef LOCAL
            fputc('.', fout);
            fputc('\n', fout);
#else
            fputc('.', stdout);
            fputc('\n', stdout);
#endif
            index1++;
        }
        printOutput(index1 - 1, index2);
    }

    return 0;
}

void redoHandler(int commandsToRedo) {
    if (redoableListFirst->commandType == 2) {
        inputText = realloc(inputText,
                            (inputTextDimension + (redoableListFirst->command[1] - redoableListFirst->command[0] + 1)) *
                            sizeof(char *));
        inputTextMemoryDimension = inputTextDimension + (redoableListFirst->command[1] - redoableListFirst->command[0] + 1);
    }
    for (int i = 0; i < commandsToRedo; i++) {
        if (redoableListFirst->commandType == 1) {
            commandHandler(redoableListFirst->commandType, redoableListFirst->command[0], redoableListFirst->command[1],
                           true);
        } else if (redoableListFirst->commandType == 2) {
            commandHandler(redoableListFirst->commandType, redoableListFirst->command[0], redoableListFirst->command[1],
                           false);
        }

        deleteNodeRedoable();
        undoRedoable--;
        redoToDo--;
    }
}

void undoHandler(int commandsToUndo) {
    if (commandsToUndo >= undoNodeCounter) {
        int toUndoCounter = undoNodeCounter;

        for (int i = 0; i < toUndoCounter; i++) {
            switch (undoableListFirst->commandType) {
                case c_command:
                    undoChange();
                    break;
                case d_command:
                    undoDelete();
                    break;
            }
        }

        undoRedoable += toUndoCounter;
    } else {
        for (int i = 0; i < commandsToUndo; i++) {
            switch (undoableListFirst->commandType) {
                case c_command:
                    undoChange();
                    break;
                case d_command:
                    undoDelete();
                    break;
            }
        }

        undoRedoable += commandsToUndo;
    }

    undoToDo = 0;
}

void undoRedoCounter(const char *readline) {
    char *ptr;
    int index = (int) strtol(readline, &ptr, 10);

    if (currentCommand == 4) {
        if (index >= redoToDo) {
            index -= redoToDo;
            redoToDo = 0;
            if (index >= undoNodeCounter || index + undoToDo >= undoNodeCounter) {
                undoToDo = undoNodeCounter;
            } else {
                undoToDo = undoToDo + index;
            }
        } else {
            redoToDo -= index;
        }
    } else if (currentCommand == 5) {
        if (index >= undoToDo) {
            index -= undoToDo;
            undoToDo = 0;
            if (index >= redoNodeCounter || index + redoToDo >= redoNodeCounter) {
                redoToDo = redoNodeCounter;
            } else {
                redoToDo = redoToDo + index;
            }
        } else {
            undoToDo -= index;
        }
    }
}

int main() {

#ifdef LOCAL
    fp = fopen("Rolling_Back_2_input.txt", "r");
    fout = fopen("a.txt", "w");
#endif

    char readLine[1025];
    setvbuf(stdout, NULL, _IOFBF, 0);

    inputText = malloc(0);
    inputTextDimension = 0;
    inputTextMemoryDimension = 0;
    finalOutput = calloc(0, sizeof(int));
    finalOutputDimension = 0;

#ifdef LOCAL
    while (fgets(readLine, 1024, fp) != NULL) {
        currentCommand = parser(readLine);
        if (currentCommand >= 1 && currentCommand <= 3) {
            if (undoToDo > 0)
                undoHandler(undoToDo);
            if (redoToDo > 0) {
                if (redoToDo <= undoRedoable)
                    redoHandler(redoToDo);
                else {
                    redoHandler(undoRedoable);
                    redoToDo = 0;
                }
            }
            if (currentCommand >= 1 && currentCommand <= 2) {
                undoRedoable = 0;
                redoableListFirst = NULL;
                redoNodeCounter = 0;
                redoToDo = 0;
            }

            char *ptr;
            int index1 = strtol(readLine, &ptr, 10);
            ptr++;
            int index2 = strtol(ptr, &ptr, 10);

            if (parser(readLine) == 1 && inputTextMemoryDimension < inputTextDimension + (index2 - index1 + 1)) {
                if ((int) (inputTextMemoryDimension * 1.5) < inputTextDimension + (index2 - index1 + 1)) {
                    inputText = realloc(inputText, (inputTextDimension + (index2 - index1 + 1)) * sizeof(char *));
                    inputTextMemoryDimension = inputTextDimension + (index2 - index1 + 1);
                } else {
                    inputText = realloc(inputText, (int) (inputTextMemoryDimension * 1.5 * sizeof(char *)));
                    inputTextMemoryDimension = (int) (inputTextMemoryDimension * 1.5);
                }
            }

            commandHandler(parser(readLine), index1, index2, false);
        } else if (currentCommand == 4 || currentCommand == 5)
            undoRedoCounter(readLine);
        else if (currentCommand == 6) {
            fflush(fout);
            return 0;
        }
    }
#else
    while (fgets(readLine, 1024, stdin) != NULL) {
        currentCommand = parser(readLine);
        if (currentCommand >= 1 && currentCommand <= 3) {
            if (undoToDo > 0)
                undoHandler(undoToDo);
            if (redoToDo > 0) {
                if (redoToDo <= undoRedoable)
                    redoHandler(redoToDo);
                else {
                    redoHandler(undoRedoable);
                    redoToDo = 0;
                }
            }
            if (currentCommand >= 1 && currentCommand <= 2) {
                undoRedoable = 0;
                redoableListFirst = NULL;
                redoNodeCounter = 0;
                redoToDo = 0;
            }

            char *ptr;
            int index1 = strtol(readLine, &ptr, 10);
            ptr++;
            int index2 = strtol(ptr, &ptr, 10);

            if (parser(readLine) == 1 && inputTextMemoryDimension < inputTextDimension + (index2 - index1 + 1)) {
                if ((int) (inputTextMemoryDimension * 1.5) < inputTextDimension + (index2 - index1 + 1)) {
                    inputText = realloc(inputText, (inputTextDimension + (index2 - index1 + 1)) * sizeof(char *));
                    inputTextMemoryDimension = inputTextDimension + (index2 - index1 + 1);
                } else {
                    inputText = realloc(inputText, (int) (inputTextMemoryDimension * 1.5 * sizeof(char *)));
                    inputTextMemoryDimension = (int) (inputTextMemoryDimension * 1.5);
                }
            }

            commandHandler(parser(readLine), index1, index2, false);
        } else if (currentCommand == 4 || currentCommand == 5)
            undoRedoCounter(readLine);
        else if (currentCommand == 6) {
            fflush(stdout);
            return 0;
        }
    }

#endif

    return 0;
}