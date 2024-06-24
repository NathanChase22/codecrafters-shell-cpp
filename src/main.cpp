#include "main.h"




int main(int argc, char** argv) {
  // REPL (read eval print loop)
  // when will there be a exit command other than an interrupt???

  std::vector<std::string> tokens;
  std::list<std::string> history;
  size_t rt_cmd_count;

  //before beginning, we need to load SIZE commands from history document 
  read_hist_log(history);

  while (1) { 
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Uncomment this block to pass the first stage
    std::cout << "$ ";
    //create a string variable and use it store console input
    std::string input;
    std::getline(std::cin, input);

    //input stream reads
    std::istringstream iss(input);
    //create array list or 'vector' to store tokens
    //NOTE: maybe better to create this outside the REPL and just flush at the end
    std::vector<std::string> tokens;
    std::string tok;

    while (std::getline(iss, tok, ' ')) {
      tokens.push_back(tok);
    }

    std::string cmd = tokens.at(0);
    switch (str_to_cmd(cmd))
    {
    case Commands::echo:
      std::cout << input.substr(5) << std::endl;
      break;
    
    case Commands::ext:
      try {
        int ext_code = std::stoi(tokens[1]);
        //before exiting we need to save all commands to file
        write_hist_log(history,rt_cmd_count);
        exit(ext_code);
      } catch (const std::exception& e) {
        std::cerr << "Invalid argument to exit: " << tokens[1] << std::endl;
      }
      break;
    
    case Commands::typ:
      //interpret next token to be command to be typed
      handle_type(tokens[1]);
      break;
    
    case Commands::pwd:
      //call getcwd()
      std::cout << getcwd(NULL, 0) << std::endl;
      break;
    
    case Commands::cd:
      //use chdir, assuming we have an absolute path
      if (chdir(tokens[1].c_str()) == -1 && tokens[1] != "~") {
        //fail condition
        std::cout << "cd: " << tokens[1] << ": No such file or directory" << std::endl;
      } 
      else if (tokens[1] == "~") {
        //user home directory
        chdir(getenv("HOME"));
      }
      else {
        //success set the environmental variable
        if (setenv("PWD",tokens[1].c_str(),1) != 0) {
          perror("setenv");
        }
      }
      
      break;
    //either it's an executable program or it's a unknown command
    default:
      //find out if program is an executable
      std::string exec_path = is_executable(cmd);
      //check to see if we got an empty string back, then fail condition
      if (!exec_path.empty()) {
        //fork a process
        int pid = fork();
        int stat;
        //if child then run
        if (pid == 0) handle_exec(exec_path, tokens);
        else waitpid(pid, &stat, 2);
        //wait till it's finished 
      } else {
        std::cout << input << ": command not found" << std::endl;
      }
    }
    //execution complete, add to the end of our list and clear our tokens array
    update_list(history,input,rt_cmd_count);
    tokens.clear();
  }
}

//reads the history from file in HISTFILE
void read_hist_log(std::list<std::string> &lst) {
  const char* fp = getenv("HISTFILE");
  if (!fp) {
    perror("HISTFILE not set!\n");
    return;
  }

  size_t max_size = std::stoi(getenv("HISTSIZE"));

  //open a file stream with fp
  std::ifstream file(fp);

  if (!file.is_open()) {
    perror("File could not open!\n");
    return;
  }

  std::string buff;
  size_t count = 0;
  while (std::getline(file, buff) && count++ < max_size) {
    //add to the front since our log starts from oldest to most recent
    lst.push_front(buff);
  }
  file.close();
}

void write_hist_log(std::list<std::string> &lst, size_t num_new_cmds) {
  //loop through our list up to the number of number of new cmds or to the end of the list
  const char* fp = getenv("HISTFILE");
  if (!fp) {
    perror("HISTFILE not set\n");
  }

  //open file and take cursor to the end of the file 
  std::ofstream file(fp, std::ios_base::app);

  for (size_t i = 0; i < lst.size() && i < num_new_cmds; i++) {
    //pop from the front and write to our file
    //assuming that the newest is at the front
    std::string cmd = lst.front();
    lst.pop_front();

    file.write(cmd.c_str(),cmd.size());
  }
  
  lst.empty();
  file.close();
}

void update_list(std::list<std::string> &lst, std::string& cmd, size_t &count) {
  //add new command to the front
  lst.push_front(cmd);
  //check to see if lst size is > HISTSIZE, if so then pop until they're <=
  while (lst.size() > std::stoi(getenv("HISTSIZE"))) {
    lst.pop_back();
  }
  count++;
}

//create command helper method, should I pass in a dereferenced ptr (&)?
Commands str_to_cmd(std::string str) {
  if (str == "echo") return Commands::echo;
  else if (str == "exit") return Commands::ext;
  else if (str == "type") return Commands::typ;
  else if (str == "pwd") return Commands::pwd;
  else if (str == "cd") return Commands::cd;
  else return Commands::unknown;
}

std::string is_executable(std::string cmd) {
  //get the enviromental PATH
  const char* path = getenv("PATH");

  std::istringstream iss(path);
  std::string fp;

  while (std::getline(iss,fp,':')) {
    //append to fp our cmd
    fp.append("/");
    fp.append(cmd);

    //check if the file exists
    if(access(fp.c_str(), F_OK | X_OK) == 0) return fp;
  }
  return "";
}

void handle_type(std::string arg1) {
  if (str_to_cmd(arg1) != Commands::unknown) {
      std::cout << arg1 << " is a shell builtin" << std::endl;
    } else {
      std::string exec_path = is_executable(arg1);
        
      if (!exec_path.empty()) {
        std::cout << arg1 << " is " << exec_path << std::endl;
      } else { //is empty, which means no executable
        std::cout << arg1 << ": not found" << std::endl;
      }
    }
}
//gonna call int execv(const char *pathname, char *const argv[])
void handle_exec(const std::string exec_path, const std::vector<std::string>& tokens) {
  std::vector<char*> argv(tokens.size()+1);
  //loop through and allocate deep copies of arg strings in tokens
  for (size_t i = 0; i < tokens.size(); i++) {
    argv[i] = strdup(tokens[i].c_str());
  }
  //null terminate the end
  argv[tokens.size()] = nullptr;
  //if execv failed then call error on process
  if (execv(exec_path.c_str(), argv.data()) == -1) {
    for (char* arg : argv) free(arg);
    perror("execv failed\n");
  }
}