#include <cstdlib>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <list>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <wait.h>

enum Commands {
    echo, ext, typ, pwd, cd, unknown=-1
};

Commands str_to_cmd(std::string str);
std::string is_executable(std::string cmd);
void handle_type(std::string arg1);
void handle_exec(const std::string exec_path, const std::vector<std::string>& tokens);
void read_hist_log(std::list<std::string> &lst);
void write_hist_log(std::list<std::string> &lst, size_t num_new_cmds);
void update_list(std::list<std::string> &lst, std::string& cmd, size_t &count);
