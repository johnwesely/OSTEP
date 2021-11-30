/* implementation of the UNIX utility ls
// user may provide optional "-l" flag for detailed information of files in directory
// if no directory is supplied by user, present working directory is used */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <time.h>

int ls(char* dir);
int lsl(char* dir);
char* get_user_name(uid_t uid);
char* get_mode_string(int m);
char* get_time(long int time);

int main(int argc, char* argv[]) {
    if (argc > 3) {
        printf("ls takes up to two arguments\n");
        return EXIT_FAILURE;
    }

    int rc;

    char pwd[1024];

    if (argc == 3) {
        rc = lsl(argv[2]);
    } else if (argc == 2 && argv[1][1] == 'l') {
        getcwd(pwd, 1024);
        rc = lsl(pwd);
    } else if (argc == 2) {
        rc = ls(argv[1]);
    } else {
        getcwd(pwd, 1024);
        rc = ls(pwd);
    }

    return rc;
}

int ls(char* dir) {
    DIR* d = opendir(dir);
    chdir(dir);

    if (!d) {
        perror("directory load fail ");
        return EXIT_FAILURE;
    }

    struct dirent* f;
    struct stat s;
    int rc;
    int m;
    while ((f = readdir(d))) {
        if (!strcmp(f->d_name, ".") || !strcmp(f->d_name, "..")) {
            continue;
        }

        rc = stat(f->d_name, &s);
        if (rc) {
            printf("%s\n", f->d_name);
            perror("ls: stat call failed: ");
            return EXIT_FAILURE;
        }
        
        m = s.st_mode;
        // if file is executable, print in red, if file is dir print in blue
        if (S_ISDIR(m)) {
            printf("\033[0;34m");
            printf("%s  ", f->d_name);
            printf("\033[0m");
        } else if (m & S_IXUSR || m & S_IXGRP || m & S_IXOTH) {
            printf("\033[0;31m");
            printf("%s  ", f->d_name);
            printf("\033[0m");
        } else {
            printf("%s  ", f->d_name);
        }
    }
    printf("\n");
    
    return EXIT_SUCCESS;
}

int lsl(char* dir) {
    DIR* d = opendir(dir);
    chdir(dir);

    if (!d) {
        perror("lsl: stat call failed: ");
        return EXIT_FAILURE;
    }

    struct dirent* f;
    struct stat s;
    int rc;
    char user_name[64];
    char* mode;
    char* time;

    while((f = readdir(d))) {
        if (!strcmp(f->d_name, ".") || !strcmp(f->d_name, "..")) {
            continue;
        }

        rc = stat(f->d_name, &s);

        if (rc) {
            perror("invalid file path ");
            return EXIT_FAILURE;
        }

        // get readable string versions of name, mode, and time
        strcpy(user_name, get_user_name(s.st_uid));
        mode = get_mode_string(s.st_mode);
        time = get_time(s.st_mtime);

        printf("%s %-4ld %s %10ld %s ", mode, s.st_nlink, user_name, s.st_size, time);
        if (S_ISDIR(s.st_mode)) { 
            printf("\033[0;34m");
            printf("%s\n", f->d_name);
            printf("\033[0m");
        } else if (s.st_mode & S_IXUSR || s.st_mode & S_IXGRP || s.st_mode & S_IXOTH) {
            printf("\033[0;31m");
            printf("%s\n", f->d_name);
            printf("\033[0m");
        } else {
            printf("%s\n", f->d_name);
        }

        free(mode);
        free(time);
    }

    return EXIT_SUCCESS;
}

// returns user name
char* get_user_name(uid_t uid) {
    struct passwd *pws;
    pws = getpwuid(uid);
    return pws->pw_name;
}

// returns mode as readable string
char* get_mode_string(int m) {
    char* mode = malloc(11);
    size_t m_i = 0;

    if (S_ISDIR(m)) {
        sprintf(mode, "d---------");
    } else {
        mode[m_i++] = '-';
        mode[m_i++] = (m & S_IRUSR) ? 'r' : '-';
        mode[m_i++] = (m & S_IWUSR) ? 'w' : '-';
        mode[m_i++] = (m & S_IXUSR) ? 'x' : '-';
        mode[m_i++] = (m & S_IRGRP) ? 'r' : '-';
        mode[m_i++] = (m & S_IWGRP) ? 'w' : '-';
        mode[m_i++] = (m & S_IXGRP) ? 'x' : '-';
        mode[m_i++] = (m & S_IROTH) ? 'r' : '-';
        mode[m_i++] = (m & S_IWOTH) ? 'w' : '-';
        mode[m_i++] = (m & S_IXOTH) ? 'x' : '-';
        mode[m_i++] = 0;
    }

    return mode;
}

// returns time as string without newline character
char* get_time(long int time) {
    char* ret = malloc(32);
    char* buff = ctime(&time);

    buff = ctime(&time);
    size_t i = 0;

    while(buff[i] != '\n') {
        ret[i] = buff[i];
        ++i;
    }

    ret[i] = 0;
    
    return ret;

}