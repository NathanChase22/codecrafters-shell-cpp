#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include "main.h"

//create command helper method, should I pass in a dereferenced ptr (&)?
Commands str_to_cmd(std::string str) {
  if (str == "echo") return Commands::echo;
  else if (str == "exit") return Commands::ext;
  else if (str == "type") return Commands::typ;
  else return Commands::unknown;
}

std::string is_executable(std::string cmd) {
  //get the enviromental PATH
  const char* path = getenv("PATH");
  std::istringstream iss(path);
  std::string fp;

  while (std::getline(iss,fp,':')) {
    //append to fp our cmd
    fp.append(cmd);
    //check if the file exists
    if(access(fp.c_str(), F_OK | X_OK) == 0) return fp;
  }
  return "";
}

int main(int argc, char** argv) {
  // REPL (read eval print loop)
  // when will there be a exit command other than an interrupt???
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
    std::vector<std::string> tokens;
    std::string tok;

    //extracts out from stream a token delimited by a space, 
    //returns input string or null once finished
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
        int ext_code = std::stoi(tokens.at(1));
        exit(ext_code);
      } catch (const std::exception& e) {
        std::cerr << "Invalid argument to exit: " << tokens.at(1) << std::endl;
      }
      break;
    
    case Commands::typ:
      //interpret next token to be command to be typed
      if (str_to_cmd(tokens.at(1)) != Commands::unknown) {
        std::cout << tokens.at(1) << " is a shell builtin" << std::endl;
      } else {
        std::cout << tokens.at(1) << ": not found" << std::endl;
      }
      break;

    default:
      //either it's an unknown command or an executible
      std::string exec_path = is_executable(cmd);
      if (!exec_path.empty()) {
        std::cout << cmd << "is " << exec_path << std::endl;
      } else { //is empty, which means no executable
        std::cout << input << ": command not found" << std::endl;
      }
      break;
    }
  }
}