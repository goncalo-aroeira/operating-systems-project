#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


char *str1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *str2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *path = "/f1";

char output1[30];
char output2[30];
int file_id;

void *func_read(void *arg){
    char *out = (char *)arg;

    tfs_read(file_id, out, strlen(str1));
    
    return NULL;
        
}

int main() {

    pthread_t tid[2];
    

    tfs_init();
    file_id = tfs_open(path, TFS_O_CREAT);
    tfs_write(file_id, str1, strlen(str1));
    tfs_close(file_id);

    file_id = tfs_open(path, 0);
    assert(file_id != -1);

    
    if (pthread_create (&tid[0], NULL, func_read, (void*)output1)){
    	exit(EXIT_FAILURE);
    }
    if (pthread_create (&tid[1], NULL, func_read, (void*)output2)){
    	exit(EXIT_FAILURE);
    }
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    
    
    assert(strcmp(output1, str1) == 0 || strcmp(output2, str1) == 0);

    assert(tfs_close(file_id) != -1);

    printf("Successful test\n");

    exit(EXIT_SUCCESS);

}