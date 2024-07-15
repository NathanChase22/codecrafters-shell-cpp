#include "main.h"

/**
 * TODO: make sure our naming conventions follow a readable standard, since right now they 
 *      all follow this_standard
 * TODO: Whenever I make a deletion in the middle of the statement it doesn't move any procedding characters to the left
 *      and vice versa
 */

//global variable which will be the termios struct
//since we have no multi threading, we can get away with a global variable
termios term, newt;

int main(int argc, char** argv) {
  // REPL (read shell_eval print loop)
  // when will there be a exit command other than an interrupt???

  std::vector<std::string> tokens;
  size_t rt_cmd_count;

  //initialize GNU History management
  init_hist();

  tcgetattr(STDIN_FILENO,&term);
  newt = term;

  // Disable canonical mode and echo
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  while (1) { 
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    //cyan foreground with black background
    std::cout << "\033[38;5;6m";
    std::cout << "$ ";
    std::cout << "\033[0m";

    //create a string variable and use it store console input
    std::string input = shell_read();

    shell_interpret(input, tokens);
    //shell_evaluate...
    shell_eval(input, tokens);
    //execution complete, add to the end of our list and clear our tokens array
    /** NOTE: Commented out to test reading only */
    add_history(input.c_str());
    history_set_pos(history_length);
    rt_cmd_count++;
    tokens.clear();
  }
}

// Reads and shell_evaluates a line of input delimited by a EOL
std::string shell_read() {
  //create temp buffer with the maximum number of characters allowed

  /**
   * TODO: -) adjust cursor and cursor when we add a new character
   *       -) adjust cursor and cursor when we remove a character
   *       -) only change the cursor when you press the side arrow key
   *       -) if the cursor is at the cursor and we move to the right it does nothing
   *       -) if the cursor is at zero then we move to the left it does nothing
   * 
   * NOTE: when we add an entry in the middle of our buffer we need to shift everything to the right
   * NOTE: consider changing our raw char array to a std vector, dyamic resizing and handles popping easily 
   */
  std::vector<char> buff(_POSIX_MAX_CANON);
  size_t cursor = 0;
  int res = 0;
  
  //get our teminal attributes
  tcgetattr(STDIN_FILENO,&newt);
  
  //check if we're in CANON mode
  if ((newt.c_lflag & ICANON) != 0x0) {
    cursor = read(STDIN_FILENO,buff.data(),_POSIX_MAX_CANON);
    if (cursor == -1) {
      std::cerr << "Error reading from stdin\n";
      return "";
    }
    buff[cursor-1] = '\0';
  } else { //we're in noncanon mode
    char c;
    do {
      //reset cursor position
      std::cout << "\033["<<(cursor+3)<<"G";
      //take in stdin character
      c = getchar();

      // //handle backspace
      if ((c == '\b' || c == '\x7f')) {
        if (!buff.empty() && cursor > 0) {
          buff.erase(buff.begin()+(--cursor));
          //move cursor back one
          putchar('\b');
          //clear from cursor to end
          std::cout << "\033[0K";
          std::cout << std::string(buff.begin()+cursor, buff.end()); 
          //reset cursor

        }
      }
      else if (c == '\x1B') {
        //arrow keys

        handle_arrow(buff, cursor);
        //should work by giving me back '[A'
      }
      else if (c== '\n') {
        buff.insert(buff.end(),c);
        putchar(c);
      }
      else { //normal characters
        //make sure our cursor isn't out of bounds 
        try {
          buff.insert(buff.begin()+(cursor++),c);
          putchar(c);
          std::cout << "\033[0K";
          std::cout << std::string(buff.begin()+cursor, buff.end()); 
        } catch (const std::out_of_range& e) {}
      }
    } while (c != '\n');
    //get rid of newline at end
    if (buff[buff.size()-1] == '\n') buff[buff.size()-1] = '\0';
  }

  return std::string(buff.begin(), buff.end());
}

void shell_interpret(std::string& input, std::vector<std::string>& tokens)  {

  //DEBUG
  std::cout << input << std::endl;
  //input stream reads
  std::istringstream iss(input);
  //create array list or 'vector' to store tokens
  //NOTE: maybe better to create this outside the REPL and just flush at the end
  std::string tok;

  while (std::getline(iss, tok, ' ')) {
    //remove all extra spaces 
    tok.erase(std::remove(tok.begin(), tok.end(), ' '), tok.end());
    //check if the token is an environmental variable
    if (tok.front() == '$') {
      const char* env_val = getenv(tok.substr(1).c_str());
      if (env_val) tokens.push_back(env_val);
      else tokens.push_back("");
    } else if (!tok.empty()){
      tokens.push_back(tok);
    }
  }    
}

void shell_eval(std::string& input, std::vector<std::string>& tokens) {
  //I have been getting errors where because the input was empty and so the token list 
  //is empty, I get a out of range error which breaks everything, lets try not letting that 
  //happen
  std::string cmd;
  try {
    cmd = tokens.at(0);
  } catch (const std::out_of_range& e) {
    return;
  }
  
  std::string prev_cmd;

  switch (str_to_cmd(cmd))
  {
  case Commands::echo:
    //print out environmental variables
    if (tokens.size() > 1) {
      for (size_t i = 1; i < tokens.size(); i++) {
        std::cout << tokens[i];
        if (i < tokens.size()-1) std::cout << " ";
      }
    }
    std::cout << std::endl;
    break;
  
  case Commands::ext:
    try {
      int ext_code = std::stoi(tokens[1]);
      //before exiting we need to save all commands to file
      write_history(getenv("HISTFILE"));
      //then we need to reset the terminal attributes
      tcsetattr(STDIN_FILENO, TCSANOW, &term);
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
}

void init_hist() {
  //check environmental variables and init environmental variables
  std::cout << "\033[38;5;33m";
  if (!getenv("HISTFILE")) {
    std::cout << "Setting HISTFILE to /Users/school/.shell_history ...." << std::endl;
    setenv("HISTFILE","/Users/school/.shell_history",1);
    std::cout << "Setting HISTSIZE to 50 ...." << std::endl;
    setenv("HISTSIZE","50",1);
  }

  using_history();
  //limit history to 50 entries
  stifle_history(50);

  //read from our history file 
  std::cout << "Recalling History from " << getenv("HISTFILE") <<" ..." << std::endl;
  int res = read_history(getenv("HISTFILE"));
  if (res != 0) {
    std::cout << "\033[38;5;9m";
    std::cerr << "Error reading history from file: " << getenv("HISTFILE") << std::endl;
  } 
  
  if (history_length > 0) { //SUCCESS
      history_set_pos(history_length);
  }
}

//handles dealing with arrow input while in the reading stage
void handle_arrow(std::vector<char>& buff, size_t& cursor) {
  //check if next char is '['
  if (getchar() == '[') {
    //grab direction of arrow
    char arrow_dir = getchar();
    //return last entry (if up or down arrow)
    if (arrow_dir == 'A' || arrow_dir == 'B') {
      const char* tmp = (grab_entry(arrow_dir)).c_str();
      //outputting our entry is simple enough, but what about actually processing it and returning it?
      //right now we have a full string, but the user may not want to process that command
      std::cout << "\033[3G";
      // Clear from the cursor to the end of the line
      std::cout << "\033[0K";
      std::cout << tmp;
      std::cout << std::flush;

      //reset buffer
      buff.clear();
      cursor = 0;
      //copy over to buff
      if (tmp[0] != '\0')  {
        buff.assign(tmp, tmp+(strlen(tmp)));
        cursor = strlen(tmp);
      }
    } 
    // NOTE: I 
    else if (arrow_dir == 'C' &&  cursor < (buff.size()-1)) {
      //left adjust cursor
      std::cout << "\033[1C";
      cursor++;
    }
    else if (arrow_dir == 'D' && cursor > 0) {
      std::cout << "\033[1D";
      cursor--;
    }
  }
}

//takes in arrow direction, up and down will change the cursor of history log, returns back entry
//think about how to have it so that when we apply the arrow key again, it erases our entry from
//stdout (without getting rid of shell prompt) and replaces with new entry
std::string grab_entry(const char arrow_dir) {
  //grab the current offset in the history list
  //depending on the direction, we either increment or decrement the number (applying modulo of max list size)
  //to make it circular, grab entry at our new offset and now return that entry

  HIST_ENTRY* hist_entry;
  switch(arrow_dir) {
    case 'B':
      //up arrow
      hist_entry = next_history();
      break;
    case 'A':
      //down arrow
      hist_entry = previous_history();
      break;
    default:
      //side arrows, do nothing 
      hist_entry = nullptr;
      break;
  }
  //handle with offset edge cases
  size_t cur_offst = where_history();
  if (cur_offst < 1) {
    history_set_pos(1);
  } else if (cur_offst > history_length) {
    history_set_pos(history_length);
  }

  //get history entry struct, grab text and turn into string
  return (hist_entry != nullptr) ? std::string(hist_entry->line) : "";
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
