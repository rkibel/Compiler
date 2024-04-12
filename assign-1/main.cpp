#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "re.hpp"
#include "nfa.hpp"

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

    // std::cout << input;
    file.close();

    //Initialize Regex for white space and comments, remove these from input

    // // Initialize Regex for every token 
   
    // std::vector<std::pair<RegularExpression, std::string>> tokens = initTokensRE(); 

    
    // Convert to NFA data structure with converter function
    // We only care about the start node

    //...
    Literal a_to_z('a', 'z');
    Literal A_to_Z('A', 'Z');
    Literal Zero_to_9('0', '9');
    return 0;
    Union ex(a_to_z, A_to_Z); //works
    Cat ex2(a_to_z, A_to_Z);
    KleeneStar ex3(ex);
    Plus ex4(ex);
    // rematurn 0;
    std::vector<std::pair<RegularExpression, std::string>> tokens;
    tokens.push_back( std::make_pair(Plus(Zero_to_9), "Num") );
    tokens.push_back( std::make_pair (Cat( Union(a_to_z, A_to_Z), KleeneStar( Union( Union(a_to_z,A_to_Z), Zero_to_9 ))), "Id") );

    for (std::pair<RegularExpression, std::string> token: tokens) {
        std::cout << token.second << "\n";
        std::pair<Node*, Node*> tokenREPair = token.first.ConvertToNFA();
        std::cout << "Second node address: " << tokenREPair.second << "\n";
    }
    return 0;

    Node* startNode = combineTokensRE(tokens);

    return 0;
    std::string inputCopy;
    while (!input.empty()) {
        Token token( RunNFA(startNode, input, inputCopy) ) ;  
        std::cout << token.name;
        if (token.name == "Id" | token.name == "Num")  {
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