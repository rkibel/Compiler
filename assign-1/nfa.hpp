#ifndef NFA_HPP
#define NFA_HPP

#include <vector>
#include <utility>
#include <string>
#include <queue>
#include <optional>
#include <iostream>
#include <set>

struct CharRange {
    char lower;
    char upper;
    CharRange(char lower, char upper) : lower(lower), upper(upper) {};
};

struct Node
{
    std::vector<std::pair<std::optional<CharRange>, Node*>> transitions;
    bool isTerminalNode;
    std::string token;
    Node(bool isTerminalNode) : isTerminalNode(isTerminalNode) {};
};

struct Token {
    std::string name;
    int index; //for storing matched lexeme Id or Num
    int length; //for storing matched lexeme Id or Num
    Token(std::string name, int index = 0, int length = 0) : name(name), index(index), length(length) {};
}


struct State //maybe don't need
{
    // The current index represents where you are in the input string for this run
    int currentIndex;
    // The current node is a pointer to where you are in the graph
    Node *currentNode;
    friend bool operator==(const State& lhs, const State& rhs) {
        return lhs.currentNode == rhs.currentNode;
    }
    State(int currentIndex, Node* currentNode) : currentIndex(currentIndex), currentNode(currentNode) {};
};

void GoAllEpsilon(std::set<Node*>& currNodes);

void TransitionState(std::set<Node*>& currNodes, char input);

    // Add any of the next states of the currentState to the nextStates queue
    // For character transition make sure to increment index, for epsilon transition make sure not to increment index
    // Return true if reaches terminal node and input is exhausted

std::vector<std::pair<RegularExpression, std::string> initTokensRE();

Node* combineTokensRE(std::vector<std::pair<RegularExpression, std::string> tokens);

std::string RunNFA(Node* startNode, std::string& input);
    // Initialize nextStates queue with starting state
    // Pop off queue and run TransitionState on dequeued state
    // Continue until empty queue or TransitionState returns true

#endif  // NFA_HPP