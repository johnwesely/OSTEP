// implemenation of stat utility 
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("stat takes one command line argument/n");
        return EXIT_FAILURE;
    }

    struct stat s;
    int rc;
    rc = stat(argv[1], &s);
    if (rc) {
        printf("invalid file path\n");
        return EXIT_FAILURE;
    }

    // extract filename from user input
    char name_buff[64];
    size_t buff_index = 0;
    size_t i = 0;
    while(argv[1][i]) {
        if (argv[1][i] == '/') {
            buff_index = 0;
            ++i;
            continue;
        }
        name_buff[buff_index++] = argv[1][i++];
    }
    name_buff[buff_index] = 0;

    printf("  File: %s\n", name_buff);
    printf("  size: %-16ldBlocks: %-16ldIO Block: %ld\n", s.st_blocks, s.st_blocks, s.st_blksize);
    printf("Device: %-16ldInode: %-16ldLinks: %ld\n", s.st_rdev, s.st_ino, s.st_nlink);
    printf("   Uid: %-16dGid: %d\n", s.st_uid, s.st_gid);
    printf("Access: %s", ctime(&s.st_atime));
    printf("Modify: %s", ctime(&s.st_mtime));
    printf("Change: %s", ctime(&s.st_ctime));

    return EXIT_SUCCESS;
}
