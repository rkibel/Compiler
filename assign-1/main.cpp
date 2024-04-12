#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// #include "re.hpp"
// #include "nfa.hpp"

int main(int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Requires exactly one argument: the name of the file to tokenize.\n";
        return 1;
    }

    std::ifstream file(argv[1]); //automaticaly tries to open

    if ( !file.is_open() ) { 
        std::cerr << "Couldn't read file: " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf(); // Read the entire file into the stringstream

    std::string input = buffer.str();

    std::cout << input;
    file.close();

    //Initialize Regex for white space and comments, remove these from input

    return 0;

    // // Initialize Regex for every token 
    tokens = initTokensRE(); 

    // Convert to NFA data structure with converter function
    // We only care about the start node

    //...
    Node* startNode = combineTokensRE(tokens);
    std::string inputCopy;
    while (!str.empty()) {
        Token token( RunNFA(startNode, input, inputCopy) ) ;  
        std::cout << token.name;
        if (token == "Id" | token == "Num")  {
            std::cout << "(" << inputCopy.substr(token.index, token.length) << ")";
        }
        std::cout << "\n";
    }
    

    /*
    loop through input string finding longest token, pop token from beginning of string and 
    repeat for the rest

    OR

    Keep track of NFA state when it is impossible to reach accept state 
    (e.g. reads character that has no defined transition)
    */ 




    return 0;
}