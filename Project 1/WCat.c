/* Implementation of Cat for OSTEP project 1
** prints given files to terminal 
** if no files or given does nothing and returns 0 */

#include <stdio.h>
#include <stdlib.h>

enum { buf_max = 32 };

int main(int argc, char* argv[]) {
    if (argc < 2) return 0;
    char buff[buf_max] = { 0 };
    for (size_t i = 1; i < argc; ++i) {
        FILE* file = fopen(argv[i], "r");
        if (!file) {
            printf("wcat cannot open file: %s\n", argv[i]);
            perror("");
            exit(1);
        }
            while (fgets(buff, buf_max file)) {
                printf("%s", buff);
            }
    }
    return 0;
}

