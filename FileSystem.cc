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
string *cwd_str; // global pointer to cwd string
int MAX_BUF = 1024;

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

bool check_bit(uint8_t n, int k){
    return ((1 << k) & n);
}

bool file_exists(const char *filename){
    ifstream ifile(filename);
    return ifile;
}

void command_error(string filename_str, int line_counter){
    cerr << "Command Error: " << filename_str << ", " << line_counter << endl;
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
        // read superblock
        int k = 0;
        int n = 1024;
        int fd = open(disk_path.c_str(), O_RDWR);
        uint8_t buffer[1024] = {0};
        lseek(fd, k , SEEK_SET);
        if(read(fd, buffer ,n)); // read [k, k+n) bytes

        // TODO: consistency checks
        bool consistent = true;
        int error_code = 0;
        // consistency check 1
        if (consistent){
            error_code = 1;
            // loop through the free-block list, check each bit (block)
            for (size_t i = 0; i < 16; i++){
                if (!consistent) break;
                for (size_t j =0; j < 8;j++){
                    if (!consistent) break;
                    
                    size_t block_index = i*8 + j;
                    // cout << "block_index= " << block_index << endl;
                    int allocated_count = 0;
                    // loop through all inodes, count allocated
                    for (size_t inode_start = 16; inode_start < 1024; inode_start+=8){
                        // cout << inode_start << endl;
                        if (check_bit(buffer[inode_start+5], 0)){ // in use (1)
                            if (!check_bit(buffer[inode_start+7], 0)){ // file (0)
                                uint8_t start_block = buffer[inode_start+6];
                                uint8_t file_size = buffer[inode_start+5] << 1 >> 1;
                                uint8_t end_block = start_block + file_size - 1;
                                if (start_block <= block_index && block_index <= end_block){
                                    allocated_count += 1;
                                }
                            }
                        }
                    }
                    cout << check_bit(buffer[i], j) << endl;
                    if (check_bit(buffer[i], j) == true){
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
        }
        // TODO: consistency check 2
        if (consistent){
            error_code = 2;

        }
        // TODO: consistency check 3
        if (consistent){
            error_code = 3;
        }
        // TODO: consistency check 4
        if (consistent){
            error_code = 4;
        }
        // TODO: consistency check 5
        if (consistent){
            error_code = 5;
        }
        // TODO: consistency check 6
        if (consistent){
            error_code = 6;
        }
        if (!consistent){
            cerr << "Error: File system in " << new_disk_name << " is inconsistent (error code: " << error_code << ")" << endl;
            // TODO: use the last file system mounted, if no fs mounted print
            cerr << "Error: No file system is mounted" << endl;
        }
        else{
            // TODO: load the superblock
            // TODO: set cwd_str to root
        }
    }
}

void fs_create(char name[5], int size){};

void fs_delete(char name[5]){};

void fs_read(char name[5], int block_num){};

void fs_write(char name[5], int block_num){};

void fs_buff(uint8_t buff[1024]){};

void fs_ls(void){};

void fs_resize(char name[5], int new_size){};

void fs_defrag(void){};

void fs_cd(char name[5]){};

void process_line(vector<string> token_str_vector, string filename_str, int line_counter){
    if ((token_str_vector[0].compare("M") == 0) && (token_str_vector.size() == 2)){
        fs_mount(const_cast<char*>(token_str_vector[1].c_str()));
    }
    else if ((token_str_vector[0].compare("C") == 0) && (token_str_vector.size() == 3)){
        if (token_str_vector[1].length() <= 5){
            fs_create(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("D") == 0) && (token_str_vector.size() == 2)){
        if (token_str_vector[1].length() <= 5){
            fs_delete(const_cast<char*>(token_str_vector[1].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("R") == 0) && (token_str_vector.size() == 3)){
        if ((token_str_vector[1].length() <= 5) && (atoi(token_str_vector[2].c_str())>=0) && (atoi(token_str_vector[2].c_str())<=126)){
            fs_read(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        }
    }
    else if ((token_str_vector[0].compare("W") == 0) && (token_str_vector.size() == 3)){
        if ((token_str_vector[1].length() <= 5) && (atoi(token_str_vector[2].c_str())>=0) && (atoi(token_str_vector[2].c_str())<=126)){
            fs_write(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("B") == 0) && (token_str_vector.size() == 2)){
        uint8_t input_buff[MAX_BUF] = {0};
        string input_str = token_str_vector[1];
        copy(input_str.begin(), input_str.end(), input_buff);
        fs_buff(input_buff);
    }
    else if ((token_str_vector[0].compare("L") == 0) && (token_str_vector.size() == 1)){
        fs_ls();
    }
    else if ((token_str_vector[0].compare("E") == 0) && (token_str_vector.size() == 3)){
        if (token_str_vector[1].length() <= 5){
            fs_resize(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
        } else{
            command_error(filename_str, line_counter);
        } 
    }
    else if ((token_str_vector[0].compare("O") == 0) && (token_str_vector.size() == 1)){
        fs_defrag();
    }
    else if ((token_str_vector[0].compare("Y") == 0) && (token_str_vector.size() == 2)){
        if (token_str_vector[1].length() <= 5){
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
    cwd_str = new string(""); // TODO: global current working directory

    BUFF = new uint8_t[MAX_BUF]; // clear the global buffer when program starts
    memset(BUFF, 0, MAX_BUF);

    int line_counter = 0; 
    string filename_str =  argv[1];
    string line;

    ifstream input_file(argv[1]);
    if (input_file.is_open()) {
        while (getline(input_file, line)) {
            line_counter += 1;
            vector<string> command_str_vector;
            command_str_vector = tokenize(line, " ");

            process_line(command_str_vector, filename_str, line_counter);
        }
        input_file.close();
    }

    delete [] BUFF;
    delete cwd_str;
    return 0;
}
