#include <iostream>

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

    //if recognized command then run
    if (input == "exit") {
      exit(0);
    } else { //ERROR 
      std::cout << input << ": command not found" << std::endl;
    }
  }
}
