#include <iostream>
#include <fstream>
#include <string>

// #include "re.hpp"
// #include "nfa.hpp"

void printPrefix(const std::string& x, const std::string& suffix) {
    size_t suffixIndex = x.find(suffix);
    if (suffixIndex != std::string::npos) {
        std::cout << "Prefix of '" << x << "' given the suffix '" << suffix << "': ";
        std::cout << x.substr(0, suffixIndex) << std::endl;
    } else {
        std::cout << "Suffix not found in the string." << std::endl;
    }
}

int main() {
    std::string x = "parked";
    std::string suffix = "ked";
    printPrefix(x, suffix);
    return 0;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Requires exactly one argument: the name of the file to tokenize.\n";
        return 1;
    }

    std::string input;
    std::ifstream myfile(argv[1]); //automaticaly tries to open

    if ( !myfile.is_open() ) { 
        std::cerr << "Couldn't read file: " << argv[1] << "\n";
        return 1;
    }

    std::string line;
    while (std::getline(myfile, line)) {
        input += line; // Append current line to input
    }

    std::cout << input << "\n";
    myfile.close();

    //Initialize Regex for white space and comments, remove these from input


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