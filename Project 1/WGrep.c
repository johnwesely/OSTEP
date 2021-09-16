/* Implementation of Grep for OSTEP Project 1
** searches argv[2] - argv[n] for argv[1] Regular Expression 
** if no files are given, searches standard input
** RegExp syntax:
**     [...]:
**         matches all characters and ranges within bracket
**         ranges in format x-y
**         '^' at beginning of pattern matches all characters except pattern 
**         ex: [ab0-9A-Z]: characters a, b, all numbers, all uppercase characters
**     ?:
**         matches zero of one of the next character 
**         ex: 'cool?.guy' matches both 'coolguy' and 'cool.guy'
**     *:
**         matches any number of characters before next character or pattern 
**         in regular expression 
**         matches greedily 
**         ex: 'ap*e' matches 'apple pie' 
**     \: 
**         escape character
**         disregards following '[', '?', '*', or '\'
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "RegEx.c"

int main(int argc, char* argv[]) {
    int ret = EXIT_FAILURE;

    char* needle = argv[1];
    char* lineBuff = 0;
    size_t lineBuffSize = 0;

    // if no arguments are given return EXIT_FAILURE
    if (argc == 1) {
        printf("No search term given\n");
        return ret;
    }

    // if no file arguments are given, read from stdin
    if (argc == 2) {
        while (getline(&lineBuff, &lineBuffSize, stdin)) {
            if (regExSearch(needle, lineBuff)) {
                printf("%s\n", lineBuff);
            }
        }
        ret = EXIT_SUCCESS;
        return ret;
    }
    // open each file given in argv and search line by line
    // for RegExp given in argv[1]
    // if RegExp needle is found on line, print line 
    for (int i = 2; i < argc; ++ i) {
        FILE* file = fopen(argv[i], "r");
        int lineSuccess = 0;
        if (file) {
            lineSuccess = getline(&lineBuff, &lineBuffSize, file);
            while (lineSuccess > 0) {
                if (regExSearch(needle, lineBuff)) {
                    printf("%s\n", lineBuff);
                }
                lineSuccess = getline(&lineBuff, &lineBuffSize, file);
            }
            fclose(file);
            ret = EXIT_SUCCESS;
        }
        else {
            fprintf(stderr, "could not open %s: ", argv[i]);
            perror(0);
            errno = 0;
        }
    }
    return ret;
}


