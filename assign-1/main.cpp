#include <iostream>
#include "re.hpp"
#include "nfa.hpp"

int main() {
    
    LiteralExpression a_to_z('a', 'z');
    LiteralExpression A_to_Z('A', 'Z');
    LiteralExpression Zero_to_9('0', '9');
    // Initialize Regex for every token 
    
    Plus temp;
    temp.inner = Zero_to_9;
    std::pair<Node*, Node*> Num = temp.ConvertToNFA();
    Num.second->token = "Num";

    // Convert to NFA data structure with converter function
    

    /*
    loop through input string finding longest token, pop token from beginning of string and 
    repeat for the rest

    OR

    Keep track of NFA state when it is impossible to reach accept state 
    (e.g. reads character that has no defined transition)
    */ 


    return 0;
}