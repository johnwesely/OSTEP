/* WZip compresses text files by converting 
** into an array of 5 byte entries
** the first four bytes are a uint32_t representing
** quantity of consecutive characters in file
** the last byte represents the character 
** use program in terminal by typing:
**     ~$ ./WZip filea.txt fileb.txt ... > zipfile.wzip */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

void compLine(char* line);

int main(int argc, char* argv[]) {
    int ret = EXIT_FAILURE;
    char* buf = 0; 
    size_t buffSize = 0;

    if (argc == 1) {
        printf("no files given\n");
        return ret;
    }

    for (size_t i = 1; i < argc; ++i) {
        FILE* file = fopen(argv[i], "r");
        int lineSuccess = 0;
        if (file) {
            lineSuccess = getline(&buf, &buffSize, file);
            while (lineSuccess > 0) {
                compLine(buf);
                lineSuccess = getline(&buf, &buffSize, file);
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

void compLine(char* line) {
    char current = line[0];
    uint32_t count = 1;
    
    for (size_t i = 1; line[i]; ++i) {
        // if char is same as last char in line
        // increment count and continue to next char
        if (line[i] == current) {
            ++count;
        }
        // else, write 5 byte entry to stdout and restart count
        else {
            // writes count of chars in sequence
            fwrite(&count, 4, 1, stdout);    
            // writes char
            fwrite(&current, 1, 1, stdout); 
            current = line[i];
            count = 1;
        }
    }
    
    // write new line character 
    fwrite(&count, 4, 1, stdout);
    fwrite(&current, 1, 1, stdout);
}

