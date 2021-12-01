/* implementation fo the tail system utility 
// program takes two command line arguments
// argv[1] is the number of lines you wish to print from the bottom of the file
// argv[2] is a file in the current directory */ 

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("tail takes 2 command line arguments\n./tail n file\nn = number of lines from end of file\nfile = file name");
        return EXIT_FAILURE;
    }

    struct stat s;
    int rc;
    rc = stat(argv[2], &s);
    if (rc) {
        perror("stat failure: ");
        return EXIT_FAILURE;
    }

    size_t fp = s.st_size;           // file pointer starts from final byte of file
    int line_n = atoi(argv[1]) + 1;  // number of lines to count

    int fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        perror("read failure: ");
        return EXIT_FAILURE;
    }

    char cur_char;

    while (line_n) {
        lseek(fd, fp--, SEEK_SET);  // work backwards from the end of file 
        read(fd, &cur_char, 1);     // until line_n newline characters have been encountered
        if (cur_char == '\n') --line_n;
    }

    char* ret = malloc(s.st_size - fp);
    read(fd, ret, s.st_size - fp);  // read file from the fp'th byte onwards into buffer
    printf("%s", ret);              // print
    free(ret);
    
    return EXIT_SUCCESS;
}