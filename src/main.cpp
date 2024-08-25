#include "main.h"

/**
 * TODO: fix up input redirection so that we have access to file contents as an arguement for execv()
 * 
 * TODO: add an additional redirection for cerr (i.e '2>') 
 * 
 * TODO: set up the Docker container to containerize my app
 */

/**
 * NOTE: ostream objects such as std::cout and std::cin are 
 *    intermediaries between the C++ call to print and the corresponding 
 *    FILE struct which represents the I/O destination on our OS
 *    
 *     So it usually looks like: 
 *        std::cout -> stdout FILE struct -> terminal output 
 *  
 *     However when I implement my redirection this is what happens,
 *        std::cout -> other FILE struct -> file contents 
 *      
 *     So stdout hasn't been altered, it's just our intermediary, cout is shooting things to the file 
 *     instead. But what if a program like 'cat' is trying to read from stdin when we have altered cin?
 *     It won't change anything! We've only altered the top 'layer' of the abstraction, which applications don't
 *     use. Therefore we need to change stdout FILE struct itself with freopen()  
 *     
 */


//global variable which will be the termios struct
//since we have no multi threading, we can get away with a global variable
termios term, newt;

//store the default buffers for cin and cout
std::streambuf* og_input_buff = std::cin.rdbuf();
std::streambuf* og_output_buff = std::cout.rdbuf();

int main(int argc, char** argv) {
  // REPL (read shell_eval print loop)
  // when will there be a exit command other than an interrupt???

  std::vector<std::string> tokens;
  std::map<std::string, std::string> usr_var;
  // std::map<std::string, std::string> cmd_aliases;
  size_t rt_cmd_count;

  //initialize GNU History management
  init_hist();

  tcgetattr(STDIN_FILENO,&term);
  newt = term;

  // Disable canonical mode and echo
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  //set up signal handler for SIGINT
  signal(SIGINT, handle_sigint);

  //debug, errors will now be red
  std::cerr << "\033[0;31m";

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

    shell_interpret(input, tokens,usr_var);
    //shell_evaluate...
    shell_eval(input, tokens);
    //execution complete, add to the end of our list and clear our tokens array
    /** NOTE: Commented out to test reading only */
    //reset the input and output
    std::cin.rdbuf(og_input_buff);
    std::cout.rdbuf(og_output_buff);

    add_history(input.c_str());
    history_set_pos(history_length);
    rt_cmd_count++;
    tokens.clear();
  }
}

// Reads and shell_evaluates a line of input delimited by a EOL
std::string shell_read() {
  //create temp buffer with the maximum number of characters allowed
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
        handle_backspace(buff, cursor);
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

void shell_interpret(std::string& input, std::vector<std::string>& tokens,
 std::map<std::string, std::string>& usr_var)  {
  
  //DEBUG: input
  // std::cout << input << std::endl;
  //input stream reads
  std::istringstream iss(input);
  //create array list or 'vector' to store tokens
  //NOTE: maybe better to create this outside the REPL and just flush at the end
  std::string tok;

  while (std::getline(iss, tok, ' ')) {
    //change the logic so that we can extract the user variable out from prefix and suffix
    if (tok.front() == '$') {
      tokens.push_back(get_uservar(tok, usr_var));
    }
    //NOTE: consider doing this in the evaluation stage
    else if (tok.find('=') != std::string::npos) {

      size_t break_at = tok.find('=');
      //KEY=VAL
      usr_var[tok.substr(0,break_at)] = tok.substr(break_at+1);
      //we don't need to push anything
    }
    else if (!tok.empty()){
      tokens.push_back(tok);
    }
  } 
}

//execute programs that we have interpreted from the user
void shell_eval(std::string& input, std::vector<std::string>& tokens) {
  //I have been getting errors where because the input was empty and so the token list 
  //is empty, I get a out of range error which breaks everything, lets try not letting that 
  //happen
  if (tokens.empty()) {
    return;
  }

  std::shared_ptr<std::ofstream> fout = handle_output_redir(tokens);
  std::shared_ptr<std::ifstream> fin = handle_input_redir(tokens);

  /**
   * NOTE: consider making this whole command decision tree into it's own function
   *    doing that. we could actually chain commands 
   */

  /**
   * DEBUG: print out contents here
   */
  std::string cmd_str = tokens.at(0);
  interpret_cmd(cmd_str, input, tokens);
}

void handle_sigint(int sig) {
  std::cin.rdbuf(og_input_buff);
  std::cout.rdbuf(og_output_buff);

  //restore terminal settings 
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  std::cerr << "Caught signal " << sig << ", exiting..." << std::endl;
  exit(0);
}

//read from the history file and populate our list
void init_hist() {
  //check environmental variables and init environmental variables
  std::cout << "\033[38;5;33m";
  if (!getenv("HISTFILE")) {
    std::cout << "Setting HISTFILE to /.shell_history ..." << std::endl;
    setenv("HISTFILE","/.shell_history",1);
    std::cout << "Setting HISTSIZE to 50 ..." << std::endl;
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

//handles deleting characters in the terminal and in the buffer
void handle_backspace(std::vector<char>& buff, size_t& cursor) {
  
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

//handles dealing with arrow input while in the reading stage
void handle_arrow(std::vector<char>& buff, size_t& cursor) {
  //check if next char is '['
  if (getchar() == '[') {
    //grab direction of arrow
    char arrow_dir = getchar();
    //return last entry (if up or down arrow)
    if (arrow_dir == 'A' || arrow_dir == 'B') {
      std::string arr_entry = grab_entry(arrow_dir);
      const char* tmp = arr_entry.c_str();
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
    else if (arrow_dir == 'C' &&  cursor < (buff.size())) {
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

//grabs a saved user-defined or environmental variable
std::string get_uservar(std::string& tok, std::map<std::string, std::string>& usr_var) {
  std::string key = tok.substr(1);

  std::string suffix;

  // Find the first non-alphanumeric character in the key
  size_t firstNonAlphaNum = std::find_if(key.begin(), key.end(), [](unsigned char c) {
      return !std::isalnum(c);
  }) - key.begin();

  // If found, split the key and the suffix
  if (firstNonAlphaNum != std::string::npos) {
      suffix = key.substr(firstNonAlphaNum);
      key.erase(firstNonAlphaNum);
  }


  if (usr_var.count(key) != 0) {
    return usr_var[key]+suffix;
  }
  else if(getenv(key.c_str()) != nullptr) {
    return getenv(key.c_str())+suffix;
  } else {
    return suffix;
  }
}

//searches in prompt for input redirection with a file and reassigns cin to file stream
std::shared_ptr<std::ifstream> handle_input_redir(std::vector<std::string>& tokens) {
  
  std::shared_ptr<std::ifstream> fin;
  
  //find_if returns a iterator which when subtracted with the begin() iterator will give us the idx position
  std::vector<std::string>::iterator input_redir = std::find_if(tokens.begin(), tokens.end(), [](std::string tok) {
    return tok == "<" || tok == "<<";
  });
  size_t input_redir_at = input_redir - tokens.begin();

  if (input_redir_at < tokens.size()-1) {
    std::string new_input = tokens.at(input_redir_at+1);
    
    //check if it's append mode
    std::ios_base::openmode input_mode = std::ios::in;
    if (tokens.at(input_redir_at) == "<<") {
      input_mode |= std::ios::app;
    }
    
    
    //open file into ifstream
    fin = std::make_shared<std::ifstream>(new_input.c_str(),input_mode);
    
    if (!(*fin)) {
      perror("ERROR: redirecting from null input file");
    } else { //file exists, reassigning std::cin
      //reassign std::cin to our file stream
      std::cin.rdbuf(fin->rdbuf());
    }
   
    //remove the tokens the direction tokens and file name
    tokens.erase(input_redir, input_redir+2);

    //take in everything in cin and put it into tokens

  }
  return fin;
}

/**
 * NOTE: turns out my issue is with this being passed by reference, getting altered,
 * then it being accessed by the parent function? Kinda weird ngl
 * 
 *  The memory address for the vector shouldn't change right?
 */

//searches in prompt for output redirection with a file and reassigns cout to file stream
std::shared_ptr<std::ofstream> handle_output_redir(std::vector<std::string>& tokens) {

  std::shared_ptr<std::ofstream> fout;

  std::vector<std::string>::iterator output_redir =  std::find_if(tokens.begin(), tokens.end(), [](std::string tok) {
    return tok == ">" || tok == ">>";
  });

  size_t output_redir_at = output_redir - tokens.begin();
  //ensure that ">>" is found and a corresponding text name is present
  
  if (output_redir_at < tokens.size()-1) {
    std::string new_output = tokens.at(output_redir_at+1);

    //check for append mode
    std::ios_base::openmode output_mode = std::ios::out;
    if (tokens.at(output_redir_at) == ">>") {
      output_mode |= std::ios::app;
    }
    
    /**
     * NOTE: made this into a shared pointer to ensure ofstream object exists even when the local 
     *    variable, fout, becomes out of scope. Now 
     * 
     *    Now I am trying to see if declaring the variable out side the if statement would work 
     */

    fout = std::make_shared<std::ofstream>(new_output.c_str(),output_mode);
    

    if (!(*fout)) {
      /** NOTE: should create the file if it doesnt exist */
      std::cerr << "ERROR" << std::endl;
      perror("ERROR: redirecting to null output file");
    } else { //file exists, reassigining std:cout

      /**
       * NOTE: what is likely happening is because fout is a local variable, once it goes 
       *      out of scope then the the buffer pointed to also disappears?
       */

      std::cout.rdbuf(fout->rdbuf());
    }
    //might just be the same as doing output_redir
    tokens.erase(tokens.begin() + output_redir_at, tokens.begin() + output_redir_at +2);
    // Debugging information
  }
  return fout;
}

//handles the cmd interpretation and execution
void interpret_cmd(std::string cmd_str, std::string& input, std::vector<std::string>& tokens) {
  Commands cmd = str_to_cmd(cmd_str);
  switch (cmd)
  {
    /**
     * NOTE: change echo so that it can read from input file directory instead of tokens 
     */
  case Commands::echo:
  
    if (tokens.size() > 1) {
      for (size_t i = 1; i < tokens.size(); i++) {
        std::cout << tokens[i];
        
        //insert a space between tokens, but not at end
        if (i < tokens.size()-1) {
          std::cout << " ";
        }
        
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
    /** NOTE: changed the logic for ~ to only look at the first char, something different with the input ~  */
    if (tokens[1].at(0) == '~') {
      //user home directory
      char* home_dir = getenv("HOME");
      //NOTE: replace '~' with home_dir
      tokens[1].erase(0,1);
      tokens[1].insert(0,home_dir);
    }


    if (chdir(tokens[1].c_str()) == -1) {
      //fail condition
      std::cerr << "cd: " << tokens[1] << ": No such file or directory" << std::endl;
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
    /**
     * NOTE: issue right now with "cat < hello.txt" is that cat is running in a child process
     *    and the child process has the default file descriptors 
     */

    //find out if program is an executable
    std::string exec_path = is_executable(cmd_str);
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
      std::cerr << input << ": command not found" << std::endl;
    }
  }
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
        std::cerr << arg1 << ": not found" << std::endl;
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
