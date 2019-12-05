#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>

using namespace std;

uint8_t *BUFF; // pointer to the global buffer


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

void fs_mount(char *new_disk_name){

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
        fs_create(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
    }
    else if ((token_str_vector[0].compare("D") == 0) && (token_str_vector.size() == 2)){
        fs_delete(const_cast<char*>(token_str_vector[1].c_str()));
    }
    else if ((token_str_vector[0].compare("R") == 0) && (token_str_vector.size() == 3)){
        fs_read(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
    }
    else if ((token_str_vector[0].compare("W") == 0) && (token_str_vector.size() == 3)){
        fs_write(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
    }
    else if ((token_str_vector[0].compare("B") == 0) && (token_str_vector.size() == 2)){
        uint8_t input_buff[1024] = {0};
        string input_str = token_str_vector[1];
        copy(input_str.begin(), input_str.end(), input_buff);
        fs_buff(input_buff);
    }
    else if ((token_str_vector[0].compare("L") == 0) && (token_str_vector.size() == 1)){
        fs_ls();
    }
    else if ((token_str_vector[0].compare("E") == 0) && (token_str_vector.size() == 3)){
        fs_resize(const_cast<char*>(token_str_vector[1].c_str()), atoi(token_str_vector[2].c_str()));
    }
    else if ((token_str_vector[0].compare("O") == 0) && (token_str_vector.size() == 1)){
        fs_defrag();
    }
    else if ((token_str_vector[0].compare("Y") == 0) && (token_str_vector.size() == 2)){
        fs_cd(const_cast<char*>(token_str_vector[1].c_str()));
    }
    else{
        cerr << "Command Error: " << filename_str << ", " << line_counter << endl;
    }     
}

int main(int argc, char const *argv[]){
    string cwd = ""; // current working directory

    BUFF = new uint8_t[1024]; // clear the global buffer when program starts
    memset(BUFF, 0, 1024);

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
    return 0;
}
