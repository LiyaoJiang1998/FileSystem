#include "FileSystem.h"
#include <unistd.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

uint8_t *BUFF; // pointer to the global buffer
int MAX_BUF = 1024;
bool mounted = false;
Super_block *SUPER_BLOCK;
uint8_t ROOT_INDEX = 127;
uint8_t CWD_INDEX = ROOT_INDEX;
string mounted_disk_name;
string mounted_disk_path;

/**
 * Reused from assignment 1 starter code
 * @brief Tokenize a string 
 * 
 * @param str - The string to tokenize
 * @param delim - The string containing delimiter character(s)
 * @return std::vector<std::string> - The list of tokenized strings. Can be empty
 */
std::vector<std::string> tokenize(const std::string &str, const char *delim)
{
  char *cstr = new char[str.size() + 1];
  std::strcpy(cstr, str.c_str());

  char *tokenized_string = strtok(cstr, delim);

  std::vector<std::string> tokens;
  while (tokenized_string != NULL)
  {
    tokens.push_back(std::string(tokenized_string));
    tokenized_string = strtok(NULL, delim);
  }
  delete[] cstr;

  return tokens;
}

/**
 * adopted from https://www.geeksforgeeks.org/check-if-two-arrays-are-equal-or-not/
 **/
bool array_equal(uint8_t arr1[], uint8_t arr2[], int n, int m){ 
    // If lengths of array are not equal means 
    // array are not equal 
    if (n != m) 
        return false; 
  
    // compare elements 
    for (int i = 0; i < n; i++) 
        if (arr1[i] != arr2[i]) 
            return false; 
  
    // If all elements were same. 
    return true; 
} 

/**
 * Update the superblock in disk with the one in memory
 **/
void write_superblock_to_disk(){
    int k = 0;
    int n = 1024;
    int fd = open(mounted_disk_path.c_str(), O_RDWR);
    uint8_t *buffer = new uint8_t[n]; // temp buffer
    memset(buffer, 0, n);
    for (int i = 0; i < 16; i++){
        buffer[i] = SUPER_BLOCK->free_block_list[i];
    }
    for (int i = 0; i <126; i++){
        int inode_start = 16 + i*8;
        for (int name_i=0;name_i<5;name_i++){
            buffer[inode_start+name_i] = SUPER_BLOCK->inode[i].name[name_i];
        }
        buffer[inode_start+5] = SUPER_BLOCK->inode[i].used_size;
        buffer[inode_start+6] = SUPER_BLOCK->inode[i].start_block;
        buffer[inode_start+7] = SUPER_BLOCK->inode[i].dir_parent;
    }
    lseek(fd, k , SEEK_SET);
    if(write(fd, buffer ,n)); // write [k, k+n) bytes
    close(fd);
    delete[] buffer;
}

/**
 * Check for the k th bit of the uint8_t k in range [0,7]
 **/
bool test_bit(uint8_t n, int k){
    return ((1 << (7-k)) & n);
}

bool file_exists(const char *filename){
    ifstream ifile(filename);
    return ifile;
}

void command_error(string filename_str, int line_counter){
    cerr << "Command Error: " << filename_str << ", " << line_counter << endl;
}

bool consistency_check_1(uint8_t *buffer){
    bool consistent = true;
    // loop through the free-block list, check each bit (block)
    for (size_t i = 0; i < 16; i++){
        if (!consistent) break;
        for (size_t j =0; j < 8;j++){
            size_t block_index = i*8 + j;
            if (block_index == 0) continue; // skip the superblock
            if (!consistent) break;
            int allocated_count = 0;
            // loop through all inodes, count allocated
            for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
                if (test_bit(buffer[inode_start+5], 0)){ // in use (1)
                    if (!test_bit(buffer[inode_start+7], 0)){ // file (0)
                        uint8_t start_block = buffer[inode_start+6];
                        uint8_t file_size = buffer[inode_start+5] & 127;
                        uint8_t end_block = start_block + file_size - 1;
                        if (start_block <= block_index && block_index <= end_block){
                            allocated_count += 1;
                        }
                    }
                }
            }
            if (test_bit(buffer[i], j) == true){
                // marked as in use, allocated to exactly one file
                if (allocated_count != 1){
                    consistent = false;
                }
            } else{
                // marked as free, cannot be allocated to any file
                if (allocated_count != 0){
                    consistent = false;
                }
            }
        }

    }
    return consistent;
}

bool consistency_check_2(uint8_t *buffer){
    bool consistent = true;
    // The name of every file/directory must be unique in each directory.
    // loop through all inodes
    for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
        if (!consistent){
            break;
        }
        if (test_bit(buffer[inode_start+5], 0)){ // in use (1)
            uint8_t unique_name[5] = {0};
            memcpy(unique_name, buffer+inode_start, 5*sizeof(uint8_t));
            uint8_t unique_parent_dir = buffer[inode_start+7] & 127;
            int name_count = 0;
            for (size_t test_inode_start = 16; test_inode_start < 1024; test_inode_start+=8){
                if (test_bit(buffer[test_inode_start+5], 0)){ // in use (1)
                    uint8_t test_parent_dir = buffer[test_inode_start+7] & 127;
                    uint8_t test_name[5] = {0};
                    memcpy(test_name, buffer+test_inode_start, 5*sizeof(uint8_t));
                    if (test_parent_dir & unique_parent_dir){
                        // if have the same parent dir, name is same, add 1 to count
                        if (array_equal(test_name, unique_name,5,5)){
                            name_count += 1;
                        }
                    }
                }
            }
            if (name_count != 1){
                consistent = false;
            }
        }
    }
    return consistent;
}

bool consistency_check_3(uint8_t *buffer){
    bool consistent = true;
    // If the state of an inode is free, all bits in this inode must be zero. 
    // Otherwise, the name attribute stored in the inode must have at least one bit that is not zero.
    // loop through all inodes
    for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
        if (!consistent){
            break;
        }
        if (test_bit(buffer[inode_start+5], 0)){ // in use (1)
            bool at_least_one = false;
            for (size_t i = inode_start; i < inode_start+5; i++){
                if (buffer[i] | 0){
                    at_least_one = true;
                }
            }
            consistent = at_least_one;
        } else{ // free (0)
            for (size_t i = inode_start; i < inode_start+8; i++){
                if (buffer[i] & 255){
                    consistent = false;
                }
                if (!consistent){
                    break;
                }
            }
        }
    }
    return consistent;
}

bool consistency_check_4(uint8_t *buffer){
    bool consistent = true;
    // The start block of every inode that is marked as a file must have a value between 1 and 127 inclusive.
    for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
        if (!consistent){
            break;
        }
        if (test_bit(buffer[inode_start+5], 0)){ // in use (1)
            if (!test_bit(buffer[inode_start+7], 0)){ // file (0)
                uint8_t start_block = buffer[inode_start+6];
                if (!( (1<=start_block) && (start_block<=127) )){
                    consistent = false;
                }
            }
        }
    }
    return consistent;
}

bool consistency_check_5(uint8_t *buffer){
    bool consistent = true;
    // The size and start block of an inode that is marked as a directory must be zero.
    for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
        if (!consistent){
            break;
        }
        if (test_bit(buffer[inode_start+5], 0)){ // in use (1)
            if (test_bit(buffer[inode_start+7], 0)){ // directory (1)
                uint8_t start_block = buffer[inode_start+6];
                uint8_t block_size = buffer[inode_start+5] & 127;
                if ((start_block != 0) || (block_size != 0)){
                    consistent = false;
                }
            }
        }
    }
    return consistent;
}

bool consistency_check_6(uint8_t *buffer){
    bool consistent = true;
    // For every inode, the index of its parent inode cannot be 126. Moreover, if the index of the parent inode
    // is between 0 and 125 inclusive, then the parent inode must be in use and marked as a directory.
    // loop through all inodes
    for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
        if (!consistent){
            break;
        }
        if (test_bit(buffer[inode_start+5], 0)){ // in use (1)
            uint8_t parent_dir = buffer[inode_start+7] & 127;
            if (parent_dir == 126){
                consistent = false;
                continue;
            }
            else if ( (0<=parent_dir) && (parent_dir<=125) ){
                size_t parent_inode_start = 16 + 8 * parent_dir;
                if (!( (test_bit(buffer[parent_inode_start+5], 0)) && (test_bit(buffer[parent_inode_start+7], 0)) )){
                    // must be in use and marked as dir
                    consistent = false;
                    continue;
                }
            }

        }
    }
    return consistent;
}

void fs_mount(char *new_disk_name){
    char cwd[PATH_MAX];
    if(getcwd(cwd, PATH_MAX));
    string cwd_str(cwd);
    string new_disk_name_str(new_disk_name);
    string disk_path = cwd_str + "/" +new_disk_name_str;
    if (!file_exists(disk_path.c_str())){
        cerr << "Error: Cannot find disk " << new_disk_name << endl;
    } else{
        // read superblock into temp buffer
        int k = 0;
        int n = 1024;
        int fd = open(disk_path.c_str(), O_RDWR);
        uint8_t *buffer = new uint8_t[n]; // consistency temp buffer
        memset(buffer, 0, n);
        lseek(fd, k , SEEK_SET);
        if(read(fd, buffer ,n)); // read [k, k+n) bytes
        close(fd);

        // consistency checks
        bool consistent = true;
        int error_code = 0;
        // consistency check 1
        if (consistent){
            error_code = 1;
            consistent = consistency_check_1(buffer);
        }
        // consistency check 2
        if (consistent){
            error_code = 2;
            consistent = consistency_check_2(buffer);
        }
        // consistency check 3
        if (consistent){
            error_code = 3;
            consistent = consistency_check_3(buffer);
        }
        // consistency check 4
        if (consistent){
            error_code = 4;
            consistent = consistency_check_4(buffer);
        }
        // consistency check 5
        if (consistent){
            error_code = 5;
            consistent = consistency_check_5(buffer);
        }
        // consistency check 6
        if (consistent){
            error_code = 6;
            consistent = consistency_check_6(buffer);
        }

        if (!consistent){
            cerr << "Error: File system in " << new_disk_name << " is inconsistent (error code: " << error_code << ")" << endl;
            // use the last file system mounted
        }
        else{
            // load the superblock
            for (int i=0;i<16;i++){
                SUPER_BLOCK->free_block_list[i] = buffer[i];
            }
            for (int i=0;i<126;i++){
                Inode temp_node;
                int inode_start = 16+8*i;
                memcpy(temp_node.name, buffer+inode_start, 5*sizeof(uint8_t));
                temp_node.used_size = buffer[inode_start+5];
                temp_node.start_block = buffer[inode_start+6];
                temp_node.dir_parent = buffer[inode_start+7];
                SUPER_BLOCK->inode[i] = temp_node;
            }
            mounted = true;
            mounted_disk_path = disk_path;
            mounted_disk_name = new_disk_name_str;
            // set CWD_INDEX to root
            CWD_INDEX = ROOT_INDEX;
        }
        delete [] buffer;
    }
}

/**
 * Usage:   char casted_name[5]; // out parameter
 *          cast_inode_name(i, casted_name);
 **/
void cast_inode_name(int i, char* casted_name){
    for (int name_i=0; name_i<5; name_i++){
        casted_name[name_i] = SUPER_BLOCK->inode[i].name[name_i];
    }
}

/**
 *  helper for fs_create, check if name exists in cwd
 **/
bool name_exist(char name[5]){
    if ( strncmp(name, "..", 5)==0 || strncmp(name, ".", 5)==0){ // reserved names
        return true;
    }
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd
            char casted_name[5];
            cast_inode_name(i, casted_name);
            if (strncmp(name, casted_name, 5) == 0){
                return true; // name is same
            }
        }
    }
    return false;
}

bool inode_available(){
    for (int i=0; i<126;i++){
        if (test_bit(SUPER_BLOCK->inode[i].used_size, 0)==false){
            return true;
        }
    }
    return false;
}

uint8_t available_blocks(int size){
    uint8_t available_start = 0; // 0 represents not enough blocks
    uint8_t block_start = 0;
    int block_count = 0;

    for (size_t i = 0; i < 16; i++){
        for (size_t j =0; j < 8;j++){
            size_t block_index = i*8 + j;
            if (block_count >= size){
                available_start = block_start;
                return available_start;
            }
            if (!test_bit(SUPER_BLOCK->free_block_list[i], j)){ // if this block is free
                block_count += 1;
            }
            else{ // if this block is not free
                block_start = block_index+1;
                block_count = 0;
            }
        }
    }
    if (block_count >= size){
        available_start = block_start;
        return available_start;
    }
    return available_start;
}

void fs_create(char name[5], int size){
    // check inode availability
    if (!inode_available()){
        cerr << "Error: Superblock in disk " << mounted_disk_name << " is full, cannot create " << name << endl;
        return;
    }
    // check name unique
    if (name_exist(name)){ // if name already exist in cwd
        cerr << "Error: File or directory " << name << " already exists" << endl;
        return;
    }
    if (size == 0){
        // create directory
        for (int i=0; i<126;i++){
            if (test_bit(SUPER_BLOCK->inode[i].used_size, 0)==false){
                // found a free inode, use it
                for (int name_i=0; name_i<5; name_i++){
                    SUPER_BLOCK->inode[i].name[name_i] = name[name_i];
                }
                SUPER_BLOCK->inode[i].used_size = 128; // in use(1), dir(0000000)
                SUPER_BLOCK->inode[i].start_block = 0; // dir 0
                SUPER_BLOCK->inode[i].dir_parent = CWD_INDEX | 128;
                write_superblock_to_disk();
                return;
            }
        }
    }
    else{
        // check available contiguous free blocks
        int start_block = available_blocks(size);
        if (start_block == 0){ //not enough contiguous free blocks
            cerr << "Error: Cannot allocate " << size << " on " << mounted_disk_name << endl;
            return;
        }
        // no error, create file
        for (int i=0; i<126;i++){
            if (test_bit(SUPER_BLOCK->inode[i].used_size, 0)==false){
                // found a free inode, use it
                for (int name_i=0; name_i<5; name_i++){
                    SUPER_BLOCK->inode[i].name[name_i] = name[name_i];
                }
                SUPER_BLOCK->inode[i].used_size = size | 128; // in use(1), filesize
                SUPER_BLOCK->inode[i].start_block = start_block; // use the start_block found available
                SUPER_BLOCK->inode[i].dir_parent = CWD_INDEX & 127;
                // set the free_block_list used blocks bit to 1
                for (int index=start_block; index < start_block+size; index++){
                    int index_i = index / 8;
                    int index_j = index % 8;
                    SUPER_BLOCK->free_block_list[index_i] = SUPER_BLOCK->free_block_list[index_i] | (1 << (7-index_j));
                }
                write_superblock_to_disk();
                return;
            }
        }
    }

}

/**
 * Zero out a specific data block, update the free-block list in memory
 **/
void flush_block_free(int block_index){
    int k = block_index*1024;
    int n = 1024;
    int fd = open(mounted_disk_path.c_str(), O_RDWR);
    uint8_t *buffer = new uint8_t[n]; // temp buffer
    memset(buffer, 0, n);
    lseek(fd, k , SEEK_SET);
    if(write(fd, buffer ,n)); // write [k, k+n) bytes
    close(fd);
    delete[] buffer;
    // update Superblock free block list
    uint8_t free_block_i = block_index / 8;
    uint8_t free_block_j = block_index % 8;
    // set the bit to 0 indicate block is free
    SUPER_BLOCK->free_block_list[free_block_i] = SUPER_BLOCK->free_block_list[free_block_i] & ~(1 << (7-free_block_j));
}

/**
 * zero out the inode with inode_index in memory
 **/
void flush_inode(int inode_index){
    for (int name_i=0;name_i<5;name_i++){
        SUPER_BLOCK->inode[inode_index].name[name_i] = 0;
    }
    SUPER_BLOCK->inode[inode_index].used_size = 0;
    SUPER_BLOCK->inode[inode_index].start_block = 0;
    SUPER_BLOCK->inode[inode_index].dir_parent = 0;
}

void recursive_delete_dir(uint8_t parent_dir_index){
    flush_inode(parent_dir_index);
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (parent_dir_index | 128)){
            // this inode has parent dir same as input
            if (test_bit(SUPER_BLOCK->inode[i].dir_parent, 0)){ // inode is dir, recursive delete
                recursive_delete_dir(i);
            } else{ // otherwise, node i is a file
                // zero out data blocks, update free_block_list
                uint8_t block_start = SUPER_BLOCK->inode[i].start_block;
                uint8_t block_size = SUPER_BLOCK->inode[i].used_size & 127;
                for(uint8_t block_i = block_start; block_i< block_start+block_size; block_i++){
                    flush_block_free(block_i);
                }
                // zero out inode 
                flush_inode(i);
            }
        }
    }
}

void fs_delete(char name[5]){
    // check name exist
    if (!name_exist(name)){ // if name not exist in cwd
        cerr << "Error: File or directory " << name << " does not exist" << endl;
        return;
    }
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd
            char casted_name[5];
            cast_inode_name(i, casted_name);
            if (strncmp(name, casted_name, 5) == 0){
                // name is same
                if (test_bit(SUPER_BLOCK->inode[i].dir_parent, 0)){ // inode is dir, recursive delete
                    recursive_delete_dir(i);
                } else{ // inode is file, delete
                    // zero out data blocks, update free_block_list
                    uint8_t block_start = SUPER_BLOCK->inode[i].start_block;
                    uint8_t block_size = SUPER_BLOCK->inode[i].used_size & 127;
                    for(uint8_t block_i = block_start; block_i< block_start+block_size; block_i++){
                        flush_block_free(block_i);
                    }
                    // zero out inode 
                    flush_inode(i);
                }
                write_superblock_to_disk();
                return;
            }
        }
    }
}

/**
 *  helper for fs_read, check if file with name exists in cwd
 **/
bool file_exist(char name[5]){
    if ( strncmp(name, "..", 5)==0 || strncmp(name, ".", 5)==0){ // reserved names
        return false; // cannot read from these
    }
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd
            char casted_name[5];
            cast_inode_name(i, casted_name);
            if (strncmp(name, casted_name, 5) == 0){
                return true; // name is same
            }
        }
    }
    return false;
}

void fs_read(char name[5], int block_num){
    if(!file_exist(name)){
        cerr << "Error: File " << name << " does not exist" << endl;
        return;
    }
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd
            char casted_name[5];
            cast_inode_name(i, casted_name);
            if (strncmp(name, casted_name, 5) == 0){
                // name is same
                uint8_t file_size = SUPER_BLOCK->inode[i].used_size & 127;
                if (!((0<=block_num)&&(block_num<=file_size-1))){
                    cerr << "Error: " << name << " does not have block " << block_num << endl;
                    return;
                }
                // no more error, read the block
                uint8_t file_start = SUPER_BLOCK->inode[i].start_block;
                uint8_t block_index = file_start + block_num;

                int k = block_index*1024;
                int n = 1024;
                int fd = open(mounted_disk_path.c_str(), O_RDWR);
                lseek(fd, k , SEEK_SET);
                if(read(fd, BUFF ,n)); // read [k, k+n) bytes
                close(fd);
            }
        }
    }
}

void fs_write(char name[5], int block_num){
    if(!file_exist(name)){
        cerr << "Error: File " << name << " does not exist" << endl;
        return;
    }
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd
            char casted_name[5];
            cast_inode_name(i, casted_name);
            if (strncmp(name, casted_name, 5) == 0){
                // name is same
                uint8_t file_size = SUPER_BLOCK->inode[i].used_size & 127;
                if (!((0<=block_num)&&(block_num<=file_size-1))){
                    cerr << "Error: " << name << " does not have block " << block_num << endl;
                    return;
                }
                // no more error, write to the block
                uint8_t file_start = SUPER_BLOCK->inode[i].start_block;
                uint8_t block_index = file_start + block_num;

                int k = block_index*1024;
                int n = 1024;
                int fd = open(mounted_disk_path.c_str(), O_RDWR);
                lseek(fd, k , SEEK_SET);
                if(write(fd, BUFF ,n)); // write [k, k+n) bytes
                close(fd);
            }
        }
    }
}

void fs_buff(uint8_t buff[1024]){
    memset(BUFF, 0, MAX_BUF); // flush the buffer with 0
    // write new characters to BUFF
    for (int i=0; i<MAX_BUF;i++){
        BUFF[i] = buff[i];
    }
}

void fs_ls(void){
    int current_dir_size = 2; // the default . and .. diretories 
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd
            current_dir_size += 1;
        }
    }
    printf("%-5s %3d\n", ".", current_dir_size); // current dir .
    
    int parent_dir_size = 2;
    if (CWD_INDEX == 127){
        parent_dir_size = current_dir_size;
    }else{
        int parent_dir_index = SUPER_BLOCK->inode[CWD_INDEX].dir_parent & 127;
        for (int i=0; i<126;i++){
            if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (parent_dir_index | 128)){
                // this inode has parent dir same as parent_dir_index
                parent_dir_size += 1;
            }
        }
    }
    printf("%-5s %3d\n", "..", parent_dir_size); // parent dir ..
    
    // child dirs and files
    for (int i=0; i<126;i++){
        if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
            // this inode has parent dir same as cwd (child file/dir)
            char child_name[5];
            cast_inode_name(i, child_name);
            if (test_bit(SUPER_BLOCK->inode[i].dir_parent, 0)){ // inode is dir
                int child_dir_size = 2;
                for (int j=0; j<126;j++){
                    if ((SUPER_BLOCK->inode[j].dir_parent | 128) == (i | 128)){
                        // this inode has parent dir same as cwd
                        child_dir_size += 1;
                    }
                }
                printf("%-5s %3d\n", child_name, child_dir_size); // child dir
            } else{ // inode is file
                int child_file_size = SUPER_BLOCK->inode[i].used_size & 127;
                printf("%-5s %3d KB\n", child_name, child_file_size); // child file
            }
        }
    }
}

void fs_resize(char name[5], int new_size){
    // TODO
}

void fs_defrag(void){
    // TODO
}

void fs_cd(char name[5]){
    if (strncmp(name, "..", 5)==0){
        if (CWD_INDEX == ROOT_INDEX){
            CWD_INDEX = CWD_INDEX;
        } else{
            CWD_INDEX = SUPER_BLOCK->inode[CWD_INDEX].dir_parent & 127;
        }
    }
    else if (strncmp(name, ".", 5)==0){
        CWD_INDEX = CWD_INDEX;
    }
    else{
        // child dirs and files
        for (int i=0; i<126;i++){
            if ((SUPER_BLOCK->inode[i].dir_parent | 128) == (CWD_INDEX | 128)){
                // this inode has parent dir same as cwd (child file/dir)
                char child_name[5];
                cast_inode_name(i, child_name);
                if (test_bit(SUPER_BLOCK->inode[i].dir_parent, 0)){ // inode is dir
                    if (strncmp(name, child_name, 5) == 0){ // dir with same name as input
                        CWD_INDEX = i;
                        return;
                    }
                } else{ // inode is file
                    cerr << "Error: Directory " << name << " does not exist" << endl;
                    return;
                }
            }
        }
        cerr << "Error: Directory " << name << " does not exist" << endl;
    }
}

void process_line(vector<string> token_str_vector, string filename_str, int line_counter, string line){    
    if ((token_str_vector[0].compare("M") == 0) && (token_str_vector.size() == 2)){
        fs_mount(const_cast<char*>(token_str_vector[1].c_str()));
    }
    else if ((token_str_vector[0].compare("C") == 0) && (token_str_vector.size() == 3)){
        if (token_str_vector[1].length() <= 5){
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            fs_create(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("D") == 0) && (token_str_vector.size() == 2)){
        if (token_str_vector[1].length() <= 5){
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            fs_delete(const_cast<char*>(token_str_vector[1].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("R") == 0) && (token_str_vector.size() == 3)){
        if ((token_str_vector[1].length() <= 5) && (atoi(token_str_vector[2].c_str())>=0) && (atoi(token_str_vector[2].c_str())<=126)){
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            fs_read(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        }
    }
    else if ((token_str_vector[0].compare("W") == 0) && (token_str_vector.size() == 3)){
        if ((token_str_vector[1].length() <= 5) && (atoi(token_str_vector[2].c_str())>=0) && (atoi(token_str_vector[2].c_str())<=126)){
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            fs_write(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("B") == 0) && (token_str_vector.size() >= 2)){
        // remove 0,1 ("B ") of line to get characters
        bool up_to = true;
        size_t pos = line.find(token_str_vector[1]);
        string characters = line.substr(pos);
        if (characters.length()<=1024){
            up_to = true;
        } else{
            up_to = false;
        }
        
        if (up_to){ // new buffer characters up to 1024
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            uint8_t input_buff[MAX_BUF] = {0};
            copy(characters.begin(), characters.end(), input_buff);
            fs_buff(input_buff);
        } else{ // more than 1024 characters are provided
            command_error(filename_str, line_counter);
        }

    }
    else if ((token_str_vector[0].compare("L") == 0) && (token_str_vector.size() == 1)){
        if (!mounted){
            cerr << "Error: No file system is mounted" << endl;
            return;
        }
        fs_ls();
    }
    else if ((token_str_vector[0].compare("E") == 0) && (token_str_vector.size() == 3)){
        if (token_str_vector[1].length() <= 5){
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            fs_resize(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("O") == 0) && (token_str_vector.size() == 1)){
        if (!mounted){
            cerr << "Error: No file system is mounted" << endl;
            return;
        }
        fs_defrag();
    }
    else if ((token_str_vector[0].compare("Y") == 0) && (token_str_vector.size() == 2)){
        if (token_str_vector[1].length() <= 5){
            if (!mounted){
                cerr << "Error: No file system is mounted" << endl;
                return;
            }
            fs_cd(const_cast<char*>(token_str_vector[1].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else{
        // invalid command or wrong number of arguments
        command_error(filename_str, line_counter);
    }     
}

int main(int argc, char const *argv[]){
    mounted_disk_path = "";
    mounted_disk_name = "";
    BUFF = new uint8_t[MAX_BUF]; // clear the global buffer when program starts
    memset(BUFF, 0, MAX_BUF);
    SUPER_BLOCK = new Super_block;

    int line_counter = 0; 
    string filename_str =  argv[1];
    string line;

    ifstream input_file(argv[1]);
    if (input_file.is_open()) {
        while (getline(input_file, line)) {
            line_counter += 1;
            vector<string> command_str_vector;
            command_str_vector = tokenize(line, " ");

            process_line(command_str_vector, filename_str, line_counter, line);
        }
        input_file.close();
    }

    delete [] BUFF;
    return 0;
}
