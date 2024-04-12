#include "nfa.hpp"

void GoAllEpsilon(std::set<Node*>& currNodes) {
    std::queue<Node*> q;
    for (Node* node: currNodes) q.push(node);
    while (!q.empty()) {
        Node* node = q.front();
        q.pop();
        if (currNodes.find(node) != currNodes.end()) continue; //if it's already in currNodes, don't need to add
        currNodes.insert(node);
        for (std::pair<std::optional<CharRange>, Node*> pair: node->transitions) {
            if (pair.first == std::nullopt) q.push(pair.second);
        }
    }
}

void TransitionState(std::set<Node*>& currNodes, char input) {
    //Takes all transitions for character inputted
    /*For me:
    For all current nodes, take the transition if possible (no lambda)
    */
    // std::string token = "Error";
    std::set<Node*> newNodes;
    for (Node* node: nodes) {
        for (std::pair<std::optional<CharRange>, Node*> pair: node->transitions) {
            //If not an epsilon transition and character within range
            if (pair.first != std::nullopt && pair.first.value().lower <= input && pair.first.value().upper >= input) {
                newNode.insert(pair.second);
            }
        }
    }
    currNodes = newNodes;
}

std::vector<std::pair<RegularExpression, std::string> initTokensRE() {
    std::vector<std::pair<RegularExpression, std::string> tokens;

    Literal a_to_z('a', 'z');
    Literal A_to_Z('A', 'Z');
    Literal Zero_to_9('0', '9');

    tokens.push_back(Plus(Zero_to_9), "Num");
    tokens.push_back( Cat( Union(a_to_z, A_to_Z), KleenStar( Union( Union(a_to_z,A_to_Z), Zero_to_9 ))));
    std::vector<std::pair<std::string, std::string>> keywords = 
        {
            { "Int"          , "int" }, 
            { "Struct"       , "struct" },
            { "Nil"          , "nil" },
            { "Break"        , "break" },
            { "Continue"     , "continue" },
            { "Return"       , "return" },
            { "If"           , "if" },
            { "Else"         , "else" },
            { "While"        , "while" },
            { "New"          , "new" },
            { "Let"          , "let" },
            { "Extern"       , "extern" },
            { "Fn"           , "fn" },
            { "Address"      , "&" },
            { "Colon"        , ":" },
            { "Semicolon"    , ";" },
            { "Comma"        , "," },
            { "Underscore"   , "_" },
            { "Arrow"        , "->" },
            { "Plus"         , "+" },
            { "Dash"         , "-" },
            { "Star"         , "*" },
            { "Slash"        , "/" },
            { "Equal"        , "==" },
            { "NotEq"        , "!=" },
            { "Lt"           , "<" },
            { "Lte"          , "<=" },
            { "Gt"           , ">" },
            { "Gte"          , ">=" },
            { "Dot"          , "." },
            { "Gets"         , "=" },
            { "OpenParen"    , "(" },
            { "CloseParen"   , ")" },
            { "OpenBracket"  , "[" },
            { "CloseBracket" , "]" },
            { "OpenBrace"    , "{" },
            { "CloseBrace"    , "}" },
        }; 
    for (std::string keywordPair: keywords) {
         tokens.push_back( createStringRE(keywordPair.second), keywordPair.first );
    }    
}


Node* combineTokensRE(std::vector<std::pair<RegularExpression, std::string> tokens) {
    //Takes in pairs of regular expressions for tokens and that token's name,
    //and joins them with lambda transitions from a starting state, but keeping their final states as terminal
    //Returns starting node, since there's no "end node"
    Node* startNode = new Node(false);
    for (std::pair<RegularExpression, std::string> token: tokens) {
        std::pair<Node*, Node*> tokenREPair = token.first.ConvertToNFA();
        tokenREPair.second->token = token.second; //assigning name to final state node
        startNode->transitions.push_back(std::make_pair(std::nullopt, tokenREPair.first));
    }
    return startNode;
};

Token RunNFA(Node* startNode, std::string& input, std::string fullInput) {
    //Runs NFA of modified union (lambda transitions added to start state but final states not modified)
    //Runs each time per token
    int longest = 0; //Keeps track of longest valid subsequence, we'll continue processing input string AFTER this, calling RunNFA again
    std::string token = "Error"; //default
    std::set<Node*> nodes;
    nodes.insert(startNode);
    GoAllEpsilon(nodes); 
    //Checks if any terminal state after the first epsilon transition
    for (Node* node: nodes) {
        //If token is error, always overwrite, then if it's not error and not an id, overwrite
        if (node->isTerminalNode && node->token != "Id" || token == "Error") token = node->token;
    }
    for (int i = 0; i < input.size(); ++i) {
        //Read next input character
        TransitionState(nodes, input[i]);
        for (Node* node: nodes) {
            if (node->isTerminalNode && (node->token != "Id" || token == "Error")) {
                longest = i;
                token = node->token;
            }
        }
        if (nodes.empty()) {
            //No more nodes are left after transitioning, no need to continue processing
            break;
        }
        //More nodes left, so restart epsilon transitions
        GoAllEpsilon(nodes); 
        //Checks if any terminal state after the first epsilon transition
        for (Node* node: nodes) {
            //If token is error, always overwrite, then if it's not error and not an id, overwrite
            if (node->isTerminalNode && node->token != "Id" || token == "Error") token = node->token;
        }
    }
    //Now we have finished processing the string, and we should have the correct token in token
    //We'll make input the input after the index of the longest valid RE we found (0 if error)
    Token tokenStruct(token);
    if (token == "Id" | token == "Num")  {
        tokenStruct.index = fullInput.find(input);
        tokenStruct.length = longest;
    }
    input = input.substr(longest); //Since we process from index 0, the longest size is the index to start after
    return tokenStruct;
}
