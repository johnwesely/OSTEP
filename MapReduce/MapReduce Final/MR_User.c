#include "MR.c"

void wordCountReduce(char *key, int partition_number) {
    size_t partIndex = 0;
    size_t count;

    while (key) {
        count = 0;
        while (!strcmp(key, partitions[partition_number][partIndex].key)) {
            ++count; ++partIndex;
            if (!partitions[partition_number][partIndex].key) break;
        }
        printf("%s %zu\n", key, count);
        key = partitions[partition_number][partIndex].key;
    }
    
    return;
}

void wordCountMap(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);

    char *line = NULL;
    size_t size = 0;
    
    while (getline(&line, &size, fp) != -1) {
        char* dummy = line;
        char* token = malloc(256);
        while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
            MR_emit(token, "1");
        }
        free(token);
    }
    free(line);
    fclose(fp);
}

int main(int argc, char* argv[]) {
    
    MR_Run(argc, argv, wordCountMap, 8, wordCountReduce, 4, MR_default_hash_partition);
    return 0;
}