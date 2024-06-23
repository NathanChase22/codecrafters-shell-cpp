#include <iostream>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // Uncomment this block to pass the first stage
  std::cout << "$ ";
  
  //create a string variable and use it store console input
  std::string input;
  std::getline(std::cin, input);
}
