/* WUnzip decompresses files created by WZip
** program takes on command line argument, 
** a file that is an array of 5 byte entries
** the first 4 bytes of entries are a uint32_t 
** representing the quantity of consecutive characters
** the last byte is the character itself */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

// prints n number of char c to stdout
void printChars(uint32_t n, char c);

int main(int argc, char* argv[]) {
    int ret = EXIT_FAILURE;

    if (argc == 1 || argc > 2) {
        fprintf(stderr, "no or multiple files given");
        perror(0);
        return ret;
    }

    FILE* file = fopen(argv[1], "r");
    // variables for storing data of each 5 byte entry
    uint32_t count = 0;
    char c = 0;
    size_t success = 0;
    if (file) {
        // read count, read character, and print
        fread(&count, 4, 1, file);
        success = fread(&c, 1, 1, file);
        printChars(count, c);

        while (success > 0) {
            fread(&count, 4, 1, file);
            success = fread(&c, 1, 1, file);
            printChars(count, c);
        }

        fclose(file);
        ret = EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "could not open %s: ", argv[1]);
        perror(0);
        errno = 0;
    }

    return ret;
}

void printChars(uint32_t n, char c) {
    for (size_t i = 0; i < n; ++i) {
        printf("%c", c);
    }
}