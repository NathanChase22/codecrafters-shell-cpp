#include <iostream>
#include <sstream>
#include <vector>

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
    
    //if recognized command then run
    if (cmd == "echo") {
      //print out output, what's the best way to do this
      //go back to input and print a substring that disincludes the first token? 
      //find position of first ' ' delimiter
      size_t pos = input.find(' ',0);
      //check to see if the position is within bounds
      if (pos != std::string::npos) {
        std::cout << input.substr(pos+1) << std::endl;
      }
    }
    else if (cmd == "exit") {
      //try to parse parse second token and convert to int
      try {
        int ext_code = std::stoi(tokens.at(1));
        exit(ext_code);
      } catch (const std::exception& e) {
        std::cerr << "Invalid argument to exit: " << tokens.at(1) << std::endl;
      }
    } else { //ERROR 
      std::cout << input << ": command not found" << std::endl;
    }
  }
}
