#include <iostream>
#include <sstream>
#include <vector>
#include "main.h"

//create command helper method, should I pass in a dereferenced ptr (&)?
Commands str_to_cmd(std::string str) {
  if (str == "echo") return Commands::echo;
  else if (str == "exit") return Commands::ext;
  else return Commands::unknown;
}

int main() {
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

    default:
      std::cout << input << ": command not found" << std::endl;
      break;
    }
  }
}
