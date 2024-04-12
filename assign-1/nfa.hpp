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
};

struct Node
{
    std::vector<std::pair<std::optional<CharRange>, Node*>> transitions;
    bool isTerminalNode;
    std::string token;
};

struct State
{
    // The current index represents where you are in the input string for this run
    int currentIndex;
    // The current node is a pointer to where you are in the graph
    Node *currentNode;
    friend bool operator==(const State& lhs, const State& rhs) {
        return lhs.currentNode == rhs.currentNode;
    }
};

void GoAllEpsilon(std::set<Node*>& currNodes);

std::string TransitionState(std::set<Node*>& currNodes, char input);

    // Add any of the next states of the currentState to the nextStates queue
    // For character transition make sure to increment index, for epsilon transition make sure not to increment index
    // Return true if reaches terminal node and input is exhausted

bool RunNFA(Node* startNode, std::string input);
    // Initialize nextStates queue with starting state
    // Pop off queue and run TransitionState on dequeued state
    // Continue until empty queue or TransitionState returns true

#endif  // NFA_HPP