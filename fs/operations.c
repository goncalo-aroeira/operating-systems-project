#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);


    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);


        if (inode == NULL) {
            return -1;
        }

        pthread_rwlock_wrlock(&inode->lock);

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                
                
                while (inode->i_size > 0){
                    size_t offset_block =  inode->i_size / BLOCK_SIZE;
                    if (offset_block < 10){
                        
                        if (data_block_free(inode->data_block[offset_block]) == -1) {
                            pthread_rwlock_unlock(&inode->lock);
                            return -1;
                        }

                       data_block_free(inode->data_block[offset_block]);
                    }
                    else{
                        int *indirect_block = (int*) data_block_get(inode-> i_data_block);
                        if (offset_block == 10){
                            data_block_free(inode->i_data_block);
                        }
                        else{
                            if(data_block_free(indirect_block[offset_block - 10]) == -1){
                                pthread_rwlock_unlock(&inode->lock);
                                return -1;
                            }
                            data_block_free(indirect_block[offset_block - 10]);
                        }
                    }
                    inode->i_size -= BLOCK_SIZE;
                }
                inode->i_size = 0;
                
            }    

        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
        pthread_rwlock_unlock(&inode->lock);
    }
    else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);

        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } 
    else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) {return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {

    size_t to_write_initial = to_write;

    size_t still_to_write = to_write;

    open_file_entry_t *file = get_open_file_entry(fhandle);
    
    if (file == NULL) {
        return -1;
    }

    pthread_mutex_lock(&file->lock);

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);

    // Bloco atual do cursor 


    size_t offset_block =  file->of_offset / BLOCK_SIZE;

    if (inode == NULL) {
        pthread_mutex_unlock(&file->lock);
        return -1;
    }

    pthread_rwlock_wrlock(&inode->lock);

    /* Determine how many bytes to write */

    
     
    /*if (to_write + file->of_offset > 10*BLOCK_SIZE) {
        to_write = 10*BLOCK_SIZE - file->of_offset;
    }*/

    int *indirect_block = 0;

    while (still_to_write > 0) {
        size_t offset_in_block = file->of_offset % BLOCK_SIZE;

        // se o cursor tiver no inicio do ficheiro e o bloco onde escrever for direto
        if (offset_in_block == 0 && offset_block < 10) {
            /* If empty file, allocate new block */
            inode->data_block[offset_block] = data_block_alloc();
        }

        // se os blocos diretos já tiverem todos preenchidos
        else{

            // se o cursor tiver no inicio e não tiver nada escrito nos indiretos
            if(offset_in_block == 0 && offset_block == 10){
                inode-> i_data_block = data_block_alloc();
                indirect_block = (int*) data_block_get(inode-> i_data_block);
                indirect_block[offset_block - 10] = data_block_alloc();
                if (indirect_block == NULL) {
                    pthread_mutex_unlock(&file->lock);
                    pthread_rwlock_unlock(&inode->lock);
                return -1;
                }
            }

            // se o cursor tiver no inicio e ja tiver algo escrito nos blocos indiretos
            else if(offset_in_block == 0 && offset_block > 10){
                indirect_block = (int*) data_block_get(inode-> i_data_block);
                indirect_block[offset_block - 10] = data_block_alloc();
                
            }

            // se já tiver coisas escritas nos blocos indiretos
            else{
                indirect_block = (int*) data_block_get(inode-> i_data_block);
            }

         
        }

        // vamos ver se o que temos de escrever é maior ou menor que 1024
        if (still_to_write >= BLOCK_SIZE){
            to_write = BLOCK_SIZE - offset_in_block;
            still_to_write -= to_write;
        }
        else{
            if (still_to_write + offset_in_block <= BLOCK_SIZE){
                to_write = still_to_write;
                still_to_write = 0;
            }
            else{
                to_write = BLOCK_SIZE - offset_in_block;
                still_to_write -= to_write;
            }
        }

        void *block;
        if(inode->i_data_block == -1){
            block = data_block_get(inode->data_block[offset_block]);
        }
        else{
           block = data_block_get(indirect_block[offset_block - 10]);
        }
        if (block == NULL) {
            pthread_mutex_unlock(&file->lock);
            pthread_rwlock_unlock(&inode->lock);
            return -1;
        }

        /* Perform the actual write */
        memcpy(block + offset_in_block, buffer, to_write);

        /* The offset associated with the file handle is
         * incremented accordingly */
        
        file->of_offset += to_write;
        
        
        offset_block ++;
        inode->i_index ++;
    }
    
    inode->i_size += to_write_initial;

    pthread_mutex_unlock(&file->lock);
    pthread_rwlock_unlock(&inode->lock);

    return (ssize_t)to_write_initial;
}



ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    pthread_mutex_lock(&file->lock);

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        pthread_mutex_unlock(&file->lock);
        return -1;
    }

    pthread_rwlock_rdlock(&inode->lock);

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }
    size_t still_to_read = to_read;
    size_t to_read_init = to_read;

    

    int *indirect_block = 0;

    void *block = 0;

    while (still_to_read > 0) {

        size_t offset_in_block = file->of_offset % BLOCK_SIZE;

        size_t offset_block =  file->of_offset / BLOCK_SIZE;
        if (offset_block < 10){
            block = data_block_get(inode->data_block[offset_block]);
            if (block == NULL) {
                pthread_mutex_unlock(&file->lock);
                pthread_rwlock_unlock(&inode->lock);
                return -1;
            }
        }
        else{
            indirect_block = (int*) data_block_get(inode-> i_data_block);

            if (indirect_block == NULL) {
                pthread_mutex_unlock(&file->lock);
                pthread_rwlock_unlock(&inode->lock);
                return -1;
            }

            block = data_block_get(indirect_block[offset_block - 10]);

            if (block == NULL) {
                pthread_mutex_unlock(&file->lock);
                pthread_rwlock_unlock(&inode->lock);
                return -1;
            }
        }

        
        if (still_to_read >= BLOCK_SIZE){
            to_read = BLOCK_SIZE;
            still_to_read -= to_read;
        }
        else{
            to_read = still_to_read;
            still_to_read = 0;
        }

        /* Perform the actual read */
        memcpy(buffer, block + offset_in_block, to_read);
        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += to_read;

    }

    pthread_mutex_unlock(&file->lock);
    pthread_rwlock_unlock(&inode->lock);

    return (ssize_t)to_read_init;


}


int tfs_copy_to_external_fs(char const *source_path, char const *dest_path){
    FILE *destiny_file = fopen(dest_path, "w");

    if( destiny_file == NULL){
        return -1;
    }

    int fhandle = tfs_lookup(source_path);
    if (fhandle == -1){
        return -1;
    }
    
    int source = tfs_open(source_path,0);
    if(source == -1){
        return -1;
    }

    open_file_entry_t *source_open_file = get_open_file_entry(source);

    inode_t *inode = inode_get(source_open_file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    char buffer[inode->i_size];
   

    size_t read_len = (size_t) tfs_read(source,buffer,inode->i_size);

    fwrite(buffer, sizeof(char), read_len, destiny_file);
    tfs_close(source);
    fclose(destiny_file);

    return 0;
}