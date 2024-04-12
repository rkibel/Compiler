#include "re.hpp"
#include "nfa.hpp"


std::pair<Node*, Node*> LiteralExpression::ConvertToNFA () {
    Node* startNode = new Node;
    startNode->isTerminalNode = false;
    Node* endNode = new Node;
    endNode->isTerminalNode = true;
    startNode->transitions.push_back(std::make_pair(std::optional<CharRange>{literal}, endNode));
    return std::make_pair(startNode, endNode);
}

std::pair<Node*, Node*> Cat::ConvertToNFA() {
    //Finds the original final state and make that the new initial state (not accepting) for the next NFA
    std::pair r1NodePair = R1.ConvertToNFA();
    std::pair r2NodePair = R2.ConvertToNFA();
    r1NodePair.second->isTerminalNode = false;
    r1NodePair.second->transitions.push_back(std::make_pair(std::nullopt, r2NodePair.first));
    return std::make_pair(r1NodePair.first, r2NodePair.second);
}

std::pair<Node*, Node*> Union::ConvertToNFA() {
    std::pair r1NodePair = R1.ConvertToNFA();
    std::pair r2NodePair = R2.ConvertToNFA();
    
    Node* startNode = new Node;
    startNode->isTerminalNode = false;
    startNode->transitions.push_back(std::make_pair(std::nullopt, r1NodePair.first));
    startNode->transitions.push_back(std::make_pair(std::nullopt, r2NodePair.first));

    r1NodePair.second->isTerminalNode = false;
    r2NodePair.second->isTerminalNode = false;

    Node* endNode = new Node;
    endNode->isTerminalNode = true;
    r1NodePair.second->transitions.push_back(std::make_pair(std::nullopt, endNode));
    r2NodePair.second->transitions.push_back(std::make_pair(std::nullopt, endNode));
    return std::make_pair(startNode, endNode);
}

std::pair<Node*, Node*> KleeneStar::ConvertToNFA() {
    std::pair innerNodePair = inner.ConvertToNFA();

    Node* startNode = new Node;
    Node* endNode = new Node;
    startNode->isTerminalNode = false;
    endNode->isTerminalNode = true;

    startNode->transitions.push_back(std::make_pair(std::nullopt, innerNodePair.first));
    startNode->transitions.push_back(std::make_pair(std::nullopt, endNode));

    innerNodePair.second->isTerminalNode = false;
    innerNodePair.second->transitions.push_back(std::make_pair(std::nullopt, endNode));
    innerNodePair.second->transitions.push_back(std::make_pair(std::nullopt, innerNodePair.first));
    return std::make_pair(startNode, endNode);
}

std::pair<Node*, Node*> Plus::ConvertToNFA() {
    KleeneStar kleeneStarRE;
    kleeneStarRE.inner = inner;
    Cat catRE;
    catRE.R1 = inner;
    catRE.R2 = kleeneStarRE;
    std::pair plusNodePair = catRE.ConvertToNFA();
    return std::make_pair(plusNodePair.first, plusNodePair.second);
}
