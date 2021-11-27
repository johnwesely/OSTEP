#ifndef __mapreduce_h__
#define __mapreduce_h__

// key value pair data structure 
typedef struct __kvp {
    char* key;
    char* val;
} kvp_t;

// Different function pointer types used by MR
typedef void (*Mapper)(char *file_name);
typedef void (*Reducer)(char *key, int partition_number);
/* user defined reducer function is responsible for traversing 
// its assinged partition. Each partition is a malloced array 
// of key value pairs, kvp_t */ 
typedef unsigned long (*Partitioner)(char *key, int num_partitions);
typedef int (*compar)(const void *, const void*);

// functions called by user

// transfers data from map function to partitions data structure
void MR_emit(char *key, char *value);

// default hash function available to be called by user
unsigned long MR_default_hash_partition(char *key, int num_partitions);

// top level function for running map reduce
// user provides map and reduce functions
// as well as the number of desired threads for both
void MR_Run(int argc, char *argv[], 
	    Mapper map, int num_mappers, 
	    Reducer reduce, int num_reducers, 
	    Partitioner partition);

#endif // __mapreduce_h__