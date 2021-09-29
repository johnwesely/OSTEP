/* implementation of 'Wisconsin Shell' for 'Operating Systems: Three Easy Pieces'
// by Arpaci-Dusseau 
// Commands:
//     cd 'path': change directroy to given path
//     path '1' '2' 'n': reset paths variable to include all given arguments
//     print: prints current paths
//     exit: exits shell
//     'any binary': searches paths for given binary and executes if found
//     'cmd' > 'filename: pipes output of 'cmd' to given 'filename'
//     'cmd' 'arg1' 'arg2' & 'cmd' & 'cmd': runs given commands in parallel */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

// main while loop of shell 
void mainWhile(FILE* stream);
// tokenizes line of input for processing
char** tokenizeLine(char* line);
// executes binary
void callExec(char** pL, char** args);
// calls chdir() for cd command
char* callChdir(char** args, char* cd);
// returns argc of given command
size_t getArgc(char** args);
// sets path variable to paths supplied by given command
char** setPath(char** args);
// checks for existence of c in args,
// returns index of instance or zero if not found
size_t charCheck(char** args, char c);

int main(int argc, char* argv[]) {
    int ret = EXIT_FAILURE;
    printf("\033[0;35m");

    // run shell with input from stdin
    if (argc == 1) {
        mainWhile(stdin);
        exit(1);
    }
    // run shell script
    else if (argc == 2) {
        FILE* stream = fopen(argv[1], "r");
        mainWhile(stream);
    }
    else {
        fprintf(stderr, "Shell accepts 0 or 1 command line arugments\n");
        return ret;
    }
}


void mainWhile(FILE* stream) {
    char* line = malloc(128 * sizeof(char));
    char* cd = malloc(128 * sizeof(char)); 
    strcpy(cd, "/");
    chdir("/");
    // initialize path list
    char** pathList = malloc(sizeof(char*) * 2);

    pathList[0] = "/bin/";
    pathList[1] = 0;

    char** args;

    while (1) {
        printf("wish:%s> ", cd);
        line = fgets(line, 128, stream);
        // EOF
        if (!line) return;
        // if return is pressed with no input, continue
        if (line[0] == '\n') continue;
        if (line) {
            // tokenize line
            args = tokenizeLine(line);
            // change directory command
            if (!strcmp(args[0], "cd")) {
                strcpy(cd, callChdir(args, cd));
            // exit command
            } else if (!strcmp(args[0], "exit")) {
                printf("goodbye friend\n");
                exit(0);
            // path command 
            } else if (!strcmp(args[0], "path")) {
                free(pathList);
                pathList = setPath(args);
            } else if (!strcmp(args[0], "print")) {
                size_t i = 0;
                while (pathList[i]) {
                     printf("pathList[%zu] == %s\n", i, pathList[i]);
                     ++i;
                }
            // execute program command
            } else {
                callExec(pathList, args);
            }
        } else {
            fprintf(stderr, "input failed: ");
            perror("");
        }
        // free memory allocated by tokenizeLine
        free(args);
    }
}


char** tokenizeLine(char* line) {
    // TokN in len of token array 
    size_t tokN = 2;

    // count necessary number of tokens
    for (size_t i = 0; line[i]; ++i) {
        if (line[i] == ' ') ++tokN;
        // replace newline with terminal character
        if (line[i] == '\n') line[i] = 0;
    }

    char** ret = malloc(sizeof(char*) * tokN);

    // tokenize line into ret
    for (size_t i = 0; ; ++i, line = 0) {
        ret[i] = strtok(line, " ");
        if (!ret[i]) break;
    }

    return ret; 
}


void callExec(char** pL, char** args) {
    size_t parallel = charCheck(args, '&');
    // if args contains parallel commands
    if (parallel) {
        size_t fk0; 
        char* argsFirst[16];
        char* argsRest[128];
     
        // populate argsFirst and argsRest
        size_t i = 0;
        size_t iF = 0;
        size_t iR = 0;
        while (args[i]) {
            if (i < parallel) {
                argsFirst[iF] = args[i];
                ++iF; ++i;
            } else if (i == parallel) {
                argsFirst[iF] = 0;
                ++i;
            } else {
                argsRest[iR] = args[i];
                ++iR; ++i;
            }
        }
        argsRest[iR] = 0;

        // callExec on argsFirst and argsRest in parallel 
        fk0 = fork();

        if (fk0 < 0) {
            fprintf(stderr, "fork failed\n");
        } else if (fk0 == 0) {
            printf("parallel command 1\n");
            callExec(pL, argsRest);
        } else {
            printf("parallel command 2\n");
            callExec(pL, argsFirst);
        }

        wait(0);
        return;
    }

    // base case command without parallel operator 
    size_t fk;
    char pathBuf[128] = { 0 };
    size_t validPath = 0;
    // true if command contains ">"
    size_t redirect = charCheck(args, '>');
    
    // check each path in pl for accessibility 
    for (size_t i = 0; pL[i]; ++i) {
        char tempBuf[128] = { 0 };
        strcat(tempBuf, pL[i]);
        strcat(tempBuf, args[0]);
        if (!access(tempBuf, F_OK)) {
            strcpy(pathBuf, tempBuf);
            ++validPath;
            break;
        }
    }
    
    // if no accessible path is found, return
    if (!validPath) {
        printf("%s not found in path list\n", args[0]);
        return;
    }

    // if command contains redirect 
    // open file specified and remove redirect command from args
    if (redirect) {
        if (!args[redirect+1]) {
            printf("redirect character must be followed by valid filename\n");
            return;
        }
        freopen(args[redirect+1], "w", stdout);
        args[redirect] = 0;
        args[redirect+1] = 0;
    }

    fk = fork();

    if (fk < 0) {
        fprintf(stderr, "fork failed\n");
    } else if (fk == 0) {
        execv(pathBuf, args);
        fprintf(stderr, "execv failed\n");
        exit(1);
    } else {
        wait(0);
        if (redirect) freopen("/dev/tty", "w", stdout);
    }
}


size_t getArgc(char** args) {
    size_t ret = 0;
    while (args[ret]) ++ret;
    return  ret;
}


char* callChdir(char** args, char* cd) {
    size_t argCount = getArgc(args);
    char* ret = cd;

    if (argCount != 2) {
        printf("error: cd takes only one argument\n");
        return ret;
    }

    chdir("/");
    if (!chdir(args[1])) {
        ret = args[1];
        return ret;
    } else {
        printf("invalid directory\n");
        chdir(cd);
        return ret;
    }
    return ret; 
}


char** setPath(char** args) {
    size_t argCount = getArgc(args);
    char** ret = malloc(sizeof(char*) * argCount);

    // copy paths to ret
    for (size_t i = 1, j = 0; i < argCount; ++i, ++j) {
        ret[j] = malloc(sizeof(char) * 128);
        strcpy(ret[j], args[i]);
    }

    // null terminate list
    ret[argCount-1] = 0;
    return ret;
}


size_t charCheck(char** args, char c) {
    size_t i = 0;
    while (args[i]) {
        if (args[i][0] == c) return i;
        ++i;
    }
    return 0;
}
