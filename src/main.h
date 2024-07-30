#include <cstdlib>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <fcntl.h>
#include <algorithm>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <termios.h>

enum Commands {
    echo, ext, typ, pwd, cd, unknown=-1
};

std::string shell_read();
void shell_interpret(std::string& input, std::vector<std::string>& tokens,std::map<std::string, std::string>& usr_var);
void shell_eval(std::string& input, std::vector<std::string>& tokens);
Commands str_to_cmd(std::string str);
std::string is_executable(std::string cmd);
void handle_type(std::string arg1);
void handle_exec(const std::string exec_path, const std::vector<std::string>& tokens);
void handle_arrow(std::vector<char>& buff, size_t& cursor);
std::string grab_entry(const char arrow_dir);
void init_hist();
std::string get_uservar(std::string& tok, std::map<std::string, std::string>& usr_var);
void handle_backspace(std::vector<char>& buff, size_t& cursor);
