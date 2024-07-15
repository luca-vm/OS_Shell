#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>


char* paths[64];
int parChecks=1;
char** commands;
int numCommands = 0;


char error_message[30] = "An error has occurred\n";
void error(){

    write(STDERR_FILENO, error_message, strlen(error_message));

}

void parCommands(char* args[], char* fileName, int argCount) {
    int i = 0;

    while (i < parChecks) {
        char pathName[256];
        snprintf(pathName, sizeof(pathName), "%s/%s", paths[i], args[0]);

        if (access(pathName, X_OK) == 0) {
            if (fork() == 0) {
                args[argCount] = NULL;
                int file = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0700);
                dup2(file, 1);
                dup2(file, 2);
                close(file);

                execv(pathName, args);
            }
            return;
        } else {
            if (i == parChecks - 1) {
                error();
            }
        }
        i++;
    }
}



void redirect(char* args[], int argCount) {

    int i = 0;
     char** newArgs = malloc(argCount * sizeof(char*));

    while ((i < argCount) && (strcmp(args[i], ">") != 0)) {
        newArgs[i] = args[i];
        i++;
    }

    if ((i == 0) || (i == argCount - 1) || (i < argCount - 2)) {
        error();
    } else {
        char* fileName = NULL;
         int targCount = argCount;

        if (i != argCount) {
            fileName = args[argCount - 1];
            targCount = argCount - 2;
        }

        parCommands(newArgs, fileName, targCount);
    }
     free(newArgs);
}


void handleExit() {
    if (numCommands == 1) {
        exit(0);
    } else {
        error();
    }
}

void handleCd(char* tcommands[]) {
    if (numCommands == 2) {
        if (chdir(tcommands[1]) != 0) {
            error();
        }
    } else {
        error();
    }
}

void handlePath(char* tcommands[]) {
    parChecks = numCommands - 1;

    if (numCommands != 1) {
        for (int k = 0; k < numCommands - 1; k++) {
            paths[k] = tcommands[k + 1];
            strcat(paths[k], "/");
        }
    } else {
        paths[0] = NULL;
    }
}

void executeExternalCommand(char* args[]) {
    if (paths[0] == NULL) {
        error();
        return;
    }

    int i;
    for (i = 0; i < numCommands;i++) {
         char** cmdArgs = malloc(numCommands * sizeof(char*));
        int argCount = 0;

        while ((i < numCommands) && (strcmp(args[i], "&") != 0)) {
            cmdArgs[argCount++] = args[i++];
        }

        if (argCount == 0) {
            free(cmdArgs);
           return;
        }
        else {
            redirect(cmdArgs, argCount);
            free(cmdArgs);
        }
   
    }


    int status;
    while (i >= 0) {
        wait(&status);
        i--;
    }
}

void executeCommands(char* tcommands[]) {
    if (numCommands == 0) {
        return;
    }

    if (strcmp(tcommands[0], "exit") == 0) {
        handleExit();
    } else if (strcmp(tcommands[0], "cd") == 0) {
        handleCd(tcommands);
    } else if (strcmp(tcommands[0], "path") == 0) {
        handlePath(tcommands);
    } else {
        executeExternalCommand(tcommands);
    }
}

char** splitCommands(const char* input) {
    char** tCommands = NULL;
    numCommands = 0;
    int i;
    int inputLength = strlen(input);

    for (i = 0; i < inputLength; i++){

        if (input[i] == ' ') {
            continue;
        }
        else if ((input[i] == '>') || (input[i] == '&')) {
            char* temp1 = malloc(2*sizeof(char));
            temp1[0] = input[i];
            temp1[1] = '\0';
            tCommands = realloc(tCommands, (numCommands + 1) * sizeof(char*));
            tCommands[numCommands++] = temp1;
        } else {
            int j = 0;
            char* temp2 = malloc(64 * sizeof(char));

            while ((i < inputLength) && (input[i] != ' ') &&
                   (input[i] != '>') && (input[i] != '&')) {
                temp2[j++] = input[i++];
            }
            tCommands = realloc(tCommands, (numCommands + 1) * sizeof(char*));
            tCommands[numCommands++] = temp2;
            i--;
        }
    }

    return tCommands;
}




void intMode(int MainArgc, char *MainArgv[]){
    char *input = NULL;
    size_t len = 0;

	while (true) {
        printf("witsshell> ");

        input = (char*)malloc(len*sizeof(char));

        ssize_t read = getline(&input, &len, stdin);

         if (read == -1) {
            free(input);
            exit(0);
        }

        input[strcspn(input, "\n")] = '\0';

     commands = splitCommands(input);

     executeCommands(commands);
 
     free(commands); 
 
    }
    free(input);
}


void batchMode(int MainArgc, char *MainArgv[]){
    char* fName = MainArgv[1];
    FILE *file = fopen(fName, "r+");
    if (file == NULL) {
        error();
        exit(1);
    }

    char *input = NULL;
    size_t len = 0;
    ssize_t read;
    int numCommands;

    while ((read = getline(&input, &len, file)) != -1) {
        input[strcspn(input, "\n")] = '\0';
      commands = splitCommands(input);
      executeCommands(commands);
      free(commands);
    
    }
    if (read == -1) {
            free(input);
            exit(0);
        }

    fclose(file);
    free(input);
}


int main(int MainArgc, char *MainArgv[]){

    paths[0] = "/bin/";
    parChecks=1;

	if (MainArgc == 1){
		intMode(MainArgc, MainArgv);
	}
    else if (MainArgc == 2){
		batchMode(MainArgc, MainArgv);
	}
    else {
		error();
        exit(1);
	}

	return(0);
}





