#include <cstdlib>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <list>
#include <fstream>
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
void shell_interpret(std::string& input, std::vector<std::string>& tokens);
void shell_eval(std::string& input, std::vector<std::string>& tokens);
Commands str_to_cmd(std::string str);
std::string is_executable(std::string cmd);
void handle_type(std::string arg1);
void handle_exec(const std::string exec_path, const std::vector<std::string>& tokens);
void handle_arrow(std::vector<char>& buff, size_t& cursor);
std::string grab_entry(const char arrow_dir);
void init_hist();
