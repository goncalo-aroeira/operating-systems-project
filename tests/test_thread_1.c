#include "fs/operations.h"
#include <assert.h>
#include <string.h>

#define SIZE 3

typedef struct{
    char *path;
    char input[SIZE];
} args;

void *call(void *th1){

    args *th = (args*) th1; 

    int fd = tfs_open(th->path, TFS_O_CREAT | TFS_O_APPEND);
    ssize_t write = tfs_write(fd, th->input,SIZE);
    assert(write == SIZE);

    assert(tfs_close(fd) != -1);

    return NULL;
}

int main(){

    char *path = "/f1";

    pthread_t tid[2];

    char output[SIZE];

    args arguments;
    arguments.path = path;
    memset(arguments.input, 'a', SIZE);

    assert(tfs_init() != -1);
    
    int fd = tfs_open(path, TFS_O_CREAT);

    if(pthread_create(&tid[0], NULL, call,(void *) &arguments) < 0)
        exit(EXIT_FAILURE);
        
    if(pthread_create(&tid[1], NULL, call,(void*) &arguments))
        exit(EXIT_FAILURE);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    fd = tfs_open(path,0);
    assert(fd != -1);

    ssize_t read = tfs_read(fd, output, SIZE);
    assert(read== SIZE);

    tfs_read(fd, output, SIZE);
    assert(read== SIZE);

    assert(tfs_close(fd) != -1);

    tfs_destroy();

    printf("Successful test\n");

    exit(EXIT_SUCCESS);

    return 0;
}