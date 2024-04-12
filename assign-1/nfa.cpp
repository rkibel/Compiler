#include "nfa.hpp"

void GoAllEpsilon(std::set<Node*>& currNodes) {
    std::queue<Node*> q;
    for (Node* node: currNodes) q.push(node);
    while (!q.empty()) {
        Node* node = q.front();
        if (currNodes.find(node) != currNodes.end()) continue;
        currNodes.insert(node);
        for (std::pair<std::optional<CharRange>, Node*> pair: node->transitions) {
            if (pair.first == std::nullopt) q.push(pair.second);
        }
        q.pop();
    }
}

std::string TransitionState(std::set<Node*>& currNodes, char input) {
    std::string token = "Error";
    
}


bool RunNFA(Node* startNode, std::string input) {
    int longest = 0;
    std::string token = "Error";
    std::set<Node*> nodes;
    nodes.insert(startNode);
    GoAllEpsilon(nodes);
    for (Node* node: nodes) {
        if (node->isTerminalNode && node->token != "Id" || token == "Error") token = node->token;
    }
    for (int i = 0; i < input.size(); ++i) {
        TransitionState(nodes, input[i]);
        for (Node* node: nodes) {
            if (node->isTerminalNode && (node->token != "Id" || token == "Error")) {
                longest = i+1;
                token = node->token;
            }
        }
    }
}
