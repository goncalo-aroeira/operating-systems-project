#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>

#define SIZE 256
#define N 8

ssize_t len = 0;

pthread_mutex_t thread;

typedef struct {
  char *path;
  void const *buffer;
} parm_write;

typedef struct {
  char *path;
  void *buffer;
} parm_read;




void *func_write(void *args) {
  parm_write pw = *((parm_write*)args);
  char *path = pw.path;
  void const *buffer = pw.buffer;
  int fhandle = tfs_open(path, TFS_O_CREAT | TFS_O_APPEND);

  assert(fhandle != -1);
  ssize_t write = 0;
  write = tfs_write(fhandle, buffer, SIZE);
  assert(write == SIZE);
  
  pthread_mutex_lock(&thread);
  len += write;
  pthread_mutex_unlock(&thread);
  
  assert(tfs_close(fhandle) != -1);
  return NULL;
}

void *func_read(void *args) {
  parm_read pr = *((parm_read*)args);
  char *path = pr.path;
  void *buffer = pr.buffer;
  int fhandle = tfs_open(path, 0);
  if (fhandle == -1) {
    return NULL; 
  }
  ssize_t read = 0;
  read = tfs_read(fhandle, buffer, SIZE);
  if(read != 0) {
    assert(read == SIZE);
  }

  assert(tfs_close(fhandle) != -1);
  return NULL;
}


int main() {
  parm_write p_write;
  parm_read p_read;
  
  
  pthread_t tid[N];
  char *path = "/f1";
  
  char buffer1[SIZE];
  memset(buffer1, 'A', SIZE);
  char buffer2[SIZE]; 

  p_write.buffer = buffer1;
  p_write.path = path;

  p_read.buffer = buffer2;
  p_read.path = path;
  
  assert(tfs_init() != -1);

  for(int i=0; i< N; i++){
      if(i%N!=0){
        if (pthread_create(&tid[i], NULL, func_write, &p_write)<0){
          exit(EXIT_FAILURE);
        }
      }
      
      else{
        if (pthread_create(&tid[i], NULL, func_read, &p_read)<0){
          exit(EXIT_FAILURE);
        }
      }

  }
  for (int i=0; i<N; i++){
    pthread_join (tid[i], NULL);
  }
  pthread_mutex_lock(&thread);
  assert(len*N == N*SIZE*(N-1));
  pthread_mutex_unlock(&thread);
  tfs_destroy();
  printf("Successful test\n");
  exit(EXIT_SUCCESS);
}