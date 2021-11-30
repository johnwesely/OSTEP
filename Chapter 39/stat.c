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

    // convert file type and mode string 
    int m = s.st_mode;
    char mode_buff[11] = { 0 };
    size_t m_i = 0;
    if (S_ISDIR(m)) {
        sprintf(mode_buff, "d---------");
    } else {
        mode_buff[m_i++] = '-';
        mode_buff[m_i++] = (m & S_IRUSR) ? 'r' : '-';
        mode_buff[m_i++] = (m & S_IWUSR) ? 'w' : '-';
        mode_buff[m_i++] = (m & S_IXUSR) ? 'x' : '-';
        mode_buff[m_i++] = (m & S_IRGRP) ? 'r' : '-';
        mode_buff[m_i++] = (m & S_IWGRP) ? 'w' : '-';
        mode_buff[m_i++] = (m & S_IXGRP) ? 'x' : '-';
        mode_buff[m_i++] = (m & S_IROTH) ? 'r' : '-';
        mode_buff[m_i++] = (m & S_IWOTH) ? 'w' : '-';
        mode_buff[m_i++] = (m & S_IXOTH) ? 'x' : '-';
        mode_buff[m_i++] = 0;
    }

    printf("  File: %s\n", name_buff);
    printf("  size: %-16ldBlocks: %-12ldIO Block: %ld\n", s.st_blocks, s.st_blocks, s.st_blksize);
    printf("Device: %-16ldInode: %-13ldLinks: %ld\n", s.st_rdev, s.st_ino, s.st_nlink);
    printf("Access: %-16sUid: %-15dGid: %d\n", mode_buff, s.st_uid, s.st_gid);
    printf("Access: %s", ctime(&s.st_atime));
    printf("Modify: %s", ctime(&s.st_mtime));
    printf("Change: %s", ctime(&s.st_ctime));

    return EXIT_SUCCESS;
}