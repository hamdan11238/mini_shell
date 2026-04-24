#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include<fstream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
extern char **environ; // look at this on web 

namespace fs = std::filesystem;


struct Redirection {
    std::string stdout_file;
    std::string stderr_file;
    std::string stdin_file;
};

#ifdef _WIN32
constexpr char PATH_SEPARATOR = ';';
#else
constexpr char PATH_SEPARATOR = ':';
#endif

bool tab_pressed_once = false; // global variable to track if tab was pressed once
std::vector<std::string> history; // global variable to store command history
// ============================================================================
// Utility Functions
// ============================================================================

void store_in_history(const std::string& command) {
    history.push_back(command);
}

void print_history() {
    for (size_t i = 0; i < history.size(); i++) {
        std::cout << i + 1 << " " << history[i] << std::endl;
    }
}

std::string longest_common_prefix(std::vector<std::string>& v) {
    if (v.empty()) return "";

    std::string prefix = v[0];

    for (const auto& s : v) {
        int i = 0;
        while (i < prefix.size() && i < s.size() && prefix[i] == s[i]) {
            i++;
        }
        prefix = prefix.substr(0, i);
    }

    return prefix;
}
std::vector<std::string> find_executables(const std::string& prefix) {
   
    std::vector<std::string> matches;

    char* path_env = getenv("PATH");
    std::stringstream ss(path_env);
    std::string dir;

    while (getline(ss, dir, ':')) {

        if (!fs::exists(dir)) continue;

        for (auto& entry : fs::directory_iterator(dir)) {

            std::string name = entry.path().filename();

            if (name.rfind(prefix, 0) == 0) {

                auto perms = fs::status(entry).permissions();

                if ((perms & fs::perms::owner_exec) != fs::perms::none) {
                    matches.push_back(name); // Store only the part after the prefix
                }
            }
        }
    }

    return matches;
}

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    
    while (ss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}
void autocomplete(std::string& input) {
   
    if(input.empty()){
        return;
    }

        if(input == "ech"){
        input += "o ";
        std::cout << "o ";
        }
        else if(input == "exi"){
        input += "t ";
        std::cout << "t ";
        }else if(input == "typ"){
        input += "e ";
        std::cout << "e ";
        }else if(input == "pw"){
        input += "d ";
        std::cout << "d ";
        }else if(input == "c"){ 
        input += "d ";
        std::cout << "d ";
        }else{
            // here we will try to find all the executables in PATH that match the input prefix and if there is only one match we will autocomplete it, otherwise we will do nothing
           std::vector<std::string> matches = find_executables(input);
           int pref_len = input.length();

            // remove duplicates + sort
            std::sort(matches.begin(), matches.end());
            matches.erase(std::unique(matches.begin(), matches.end()), matches.end());



            if (matches.empty()) {
                std::cout << '\a';
                return;
            }


            if (matches.size() == 1) {

                std::string completion = matches[0].substr(pref_len);

                input += completion + " ";
                std::cout << completion << " ";

                tab_pressed_once = false;
                return;
            }
            std::string lcp = longest_common_prefix(matches);

            if (lcp.length() > input.length()) {

                // expand to LCP
                std::string completion = lcp.substr(input.length());

                input += completion;
                std::cout << completion;

                tab_pressed_once = false;
            }
            else{

            if (!tab_pressed_once) {
                std::cout << '\a';   // first TAB → bell
                tab_pressed_once = true;
            }
            else {
                std::cout << "\n";

                for (size_t i = 0; i < matches.size(); i++) {
                    std::cout << matches[i];
                    if (i < matches.size() - 1)
                        std::cout << "  ";   // TWO spaces
                }

                std::cout << "\n$ " << input;

                tab_pressed_once = false;
            }
            }
        }
                    return;
    
}
void enable_raw_mode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~(ICANON | ECHO);  // disable line buffering and echo

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

// every space is  preserve if its betwween single quotes, otherwise its removed
// if not in single quotes, we will remove extra spaces and keep only one space between words
// if two single qouted words we will concatenate them;
std::vector<std::string> parser(const std::string& input){
    std::vector<std::string> result;
    bool single_quotes = false;
    bool double_quotes = false;
    std::string current_token = "";

   for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];

        if( c== '"' && !single_quotes){
            double_quotes = !double_quotes;
             continue; // do NOT include quote in token
        }

       if (double_quotes) {
        if (c == '\\') {
        char next = input[i + 1];

        if (next == '\\' || next == '"' || next == '$' || next == '`') {
            current_token += next;
            i++;  // skip next character
        } else {
            current_token += '\\';
        }
    }
    else {
        current_token += c;
    }
}
        if(c == '\'' && !double_quotes){
            single_quotes = !single_quotes;
             continue; // do NOT include quote
        
            // if(!current_token.empty()){
            //     result.push_back(current_token);
            //     current_token = "";
            // }

        } else if(single_quotes){
            current_token += c;

        } else if(!single_quotes && !double_quotes){

            if(!result.empty() && result.back().empty() && c == ' '){
                continue; // Skip extra spaces
            } 
            else if(c == '\\'){
                if( i+1 < input.size()){
                i++;
                current_token += input[i]; // Include the character after the backslash
                }
                
            }

            else if(c == ' '){
                    if(!current_token.empty()){
                
                    result.push_back(current_token);
                    current_token = "";
                    
                    }
                
            } else {
                current_token += c;
            }
        }
}
    if(!current_token.empty()){
        result.push_back(current_token);
        current_token = "";
    }
     return result;
}

void print_error(const std::string& command) {
    std::cerr << command << ": not found" << std::endl;
}

// ============================================================================
// Command Type Checking
// ============================================================================

bool is_builtin(const std::string& command, const std::vector<std::string>& builtins) {
    for (const auto& builtin : builtins) {
        if (builtin == command) {
            return true;
        }
    }
    return false;
}

std::string find_executable_in_path(const std::string& command, const std::string& path_env) {
    std::stringstream ss(path_env);
    std::string directory;
    
    while (std::getline(ss, directory, PATH_SEPARATOR)) {
        if (directory.empty()) continue;
        
        fs::path candidate = fs::path(directory) / command;
        
        if (fs::exists(candidate)) {
            std::error_code errorCode;
            fs::perms permissions = fs::status(candidate, errorCode).permissions();
            
            if (errorCode) {
                continue; // Skip on error
            }
            
            // Check if executable by owner
            if ((permissions & fs::perms::owner_exec) != fs::perms::none) {
                return candidate.string();
            }
        }
    }
    
    return ""; // Not found
}

// ============================================================================
// Built-in Command Handlers
// ============================================================================
void handle_cd(std::string path) {
    if(path.empty())
    perror("path empty");

    // check if  path exists
    if(path == "./")
    return;

    if(path == "../"){
        fs::path currentPath = fs::current_path();
        fs::path parentPath = currentPath.parent_path();
        if(chdir(parentPath.c_str()) == 0)
            return;
        else
            perror("chdir failed");
       
    }

    if(path == "~"){
          const char* homeDir = std::getenv("HOME");
          if (homeDir != nullptr){
            if(chdir(homeDir) == 0)
                return;
                else
                perror("chdir failed");
           } else{
            std::cerr << "cd: HOME environment variable not set" << std::endl;
          }
    }

    if(fs::exists(path)){
    if(chdir(path.c_str()) == 0) // if path changed successfully 
        return;
    else
        perror("chdir failed");
    } else{
        std::cerr << "cd: " << path << ": No such file or directory" << std::endl;
    }
    


}
void handle_pwd(){
     std::filesystem::path currentPath = std::filesystem::current_path();
     std::cout << currentPath.string() << std::endl;
}
void handle_exit(const std::vector<std::string>& tokens) {
    int exit_code = 0;
    
    if (tokens.size() > 1) {
        exit_code = std::atoi(tokens[1].c_str());
    }
    
    std::exit(exit_code);
}

void handle_echo(const std::vector<std::string>& tokens) {
    for (size_t i = 1; i < tokens.size(); i++) {
        std::cout << tokens[i];
        if (i < tokens.size() - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}

void handle_type(const std::vector<std::string>& tokens, 
                 const std::vector<std::string>& builtins,
                 const std::string& path_env) {
    if (tokens.size() < 2) {
        std::cerr << "type: missing argument" << std::endl;
        return;
    }
    
    const std::string& command = tokens[1];
    
    // Check if it's a builtin
    if (is_builtin(command, builtins)) {
        std::cout << command << " is a shell builtin" << std::endl;
        return;
    }
    
    // Check if it exists in PATH
    std::string executable_path = find_executable_in_path(command, path_env);
    if (!executable_path.empty()) {
        std::cout << command << " is " << executable_path << std::endl;
        return;
    }
    
    // Not found
    print_error(command);
}

// ============================================================================
// Main Shell Loop
// ============================================================================

int main() {
    // Disable output buffering
    std::cout << std::unitbuf;
    
    const std::vector<std::string> BUILTIN_COMMANDS = {"echo", "exit", "type" ,"pwd","history"};
    
    while (true) {
        // Display prompt
        std::cout << "$ ";
        
        // Read input
        std::string input="";
        // std::getline(std::cin, input);
        // input = parser(input);

         enable_raw_mode(); // Enable raw mode to read input character by character
            char c;
         while(read(STDIN_FILENO, &c, 1)>0){
           
            if (c == '\t') {
              
                autocomplete(input);
                tab_pressed_once = true;
                continue;
                // auto triggered
            }
            if (c == '\n') {
             std::cout << std::endl;   // important to move to the next line after pressing enter
                break; // End of input
            }
            std::cout<<c<<std::flush;
            input += c;
        }
        

        if (input.empty()) {
            continue;
        }
        
        // Parse input into tokens
        std::vector<std::string> tokens = parser(input);
        
        if (tokens.empty()) {
            continue;
        }

        // if token contains redirection operator we need to handle it here and remove it from tokens vector and store the file name in redirection struct
        bool append_mode = false; // flag to indicate if we are in append mode (>>)
        Redirection redirection;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i] == ">" || tokens[i] == "1>") {
                if (i + 1 < tokens.size()) {
                    redirection.stdout_file = tokens[i + 1];
                    tokens.erase(tokens.begin() + i, tokens.begin() + i + 2); // Remove operator and filename from tokens
                    i--; // Adjust index after erasing
                } else {
                    std::cerr << "Syntax error: expected filename after '>'" << std::endl;
                    break;
                }   
            }else if(tokens[i] == "2>"){
                if (i + 1 < tokens.size()) {
                    redirection.stderr_file = tokens[i + 1];
                    tokens.erase(tokens.begin() + i, tokens.begin() + i + 2); // Remove operator and filename from tokens
                    i--; // Adjust index after erasing
                } else {
                    std::cerr << "Syntax error: expected filename after '>'" << std::endl;
                    break;
                }   

            }else if(tokens[i] == ">>" || tokens[i] == "1>>"){
                if (i + 1 < tokens.size()) {
                    append_mode = true;
                    redirection.stdout_file = tokens[i + 1];
                    tokens.erase(tokens.begin() + i, tokens.begin() + i + 2); // Remove operator and filename from tokens
                    i--; // Adjust index after erasing
                } else {
                    std::cerr << "Syntax error: expected filename after '>'" << std::endl;
                    break;
                }
            }else if(tokens[i] == "2>>"){
                if (i + 1 < tokens.size()) {
                    append_mode = true;
                    redirection.stderr_file = tokens[i + 1];
                    tokens.erase(tokens.begin() + i, tokens.begin() + i + 2); // Remove operator and filename from tokens
                    i--; // Adjust index after erasing
                } else {
                    std::cerr << "Syntax error: expected filename after '>'" << std::endl;
                    break;
                }
            }
    }

        // making changes for std out
            int saved_stdout = -1; 
            // now change the output  stream to the file specified in redirection.stdout_file using dup2 system call and open system call
             if (!redirection.stdout_file.empty()) {
               // Save original stdout
                saved_stdout = dup(STDOUT_FILENO);
                int fd;
                if(!append_mode)
                fd = open(redirection.stdout_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                else
                fd = open(redirection.stdout_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd == -1) {
                    perror("open");
                } else {
                    dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
                    close(fd); // Close the file descriptor after redirecting
                }
             }

        // redirecting std error changes 

        int saved_stderr = -1;

        if(!redirection.stderr_file.empty()){

            saved_stderr = dup(STDERR_FILENO);
            int ed;
                if(!append_mode)
                ed = open(redirection.stderr_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                else
                ed = open(redirection.stderr_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if(ed == -1){
                perror("open");

            }else{
                dup2(ed , STDERR_FILENO);
                close(ed);
            }
        }

             
        
        
        // Get PATH environment variable
        const char* path_ptr = getenv("PATH");
        std::string path_env = path_ptr ? path_ptr : "";
        
        if (path_env.empty()) {
            std::cerr << "PATH environment variable not set." << std::endl;
            continue;
        }
        
        // Get the command (first token)
        const std::string& command = tokens[0];
        store_in_history(input); // Store the command in history
        
        // Handle built-in commands
        if (command == "exit") {
            handle_exit(tokens);
        }
        else if (command == "echo") {
            handle_echo(tokens);
        }
        else if (command == "type") {
            handle_type(tokens, BUILTIN_COMMANDS, path_env);
        }else if (command == "pwd") {
            handle_pwd();
        }else if(command == "cd"){
            if(tokens.size() < 2){
                std::cerr << "cd: missing argument" << std::endl;
            } else{
                handle_cd(tokens[1]);
            }
        }
        else if(command == "history"){
            print_history();
        }
        else{
            // // Unknown command
            // print_error(command);

            // here we need to check if command exists in PATH or not, if it does we need to execute it using execve system call, and if does not we need to print error meassage 
                std::string executable_path = find_executable_in_path(command, path_env);
                if (executable_path.empty()) {
                    print_error(command);
                }
                else{ 
                   
                    int pid = fork();
                    if (pid == 0) {
                        // Child process
                    std::vector<char*> args;

                    for(auto& token:tokens){
                        args.push_back(const_cast<char*>(token.c_str()));

                    }
                    args.push_back(nullptr); // execve expects a null-terminated array

                    // Execute the command
                    if (execve(executable_path.c_str(), args.data(), environ) == -1) {
                        perror("execve");
                    }
                }
                else if(pid >0){
                    // Parent process
                    int status;
                    waitpid(pid, &status, 0);
                }
                else{
                    // Fork failed
                    perror("fork");
                }
                    
    
        }
    }

    if(!redirection.stdout_file.empty()){
        // Restore original stdout if it was redirected
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
}

   if(!redirection.stderr_file.empty()){
        // Restore original stdout if it was redirected
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
}
    }


    
    return 0;
}
