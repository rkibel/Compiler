#ifndef NFA_HPP
#define NFA_HPP

#include <vector>
#include <utility>
#include <string>
#include <queue>
#include <optional>

struct Node
{
    std::vector<std::pair<std::optional<CharRange>, Node*>> transitions;
    bool isTerminalNode;

    //Would be nice to have overloaded print operator to easily print the full NFA from that node (since there's no "NFA" struct now)
};

struct State
{
    // The current index represents where you are in the input string for this run
    int currentIndex;
    // The current node is
    // a pointer to where you are in the graph
    Node *currentNode;
};

bool TransitionState(State& currentState, std::string input, std::queue<State>& nextStates);
    // Add any of the next states of the currentState to the nextStates queue
    // For character transition make sure to increment index, for epsilon transition
//make sure not to increment index
    // Return true if reaches terminal node and input is exhausted

bool RunNFA(Node* startNode, std::string input);
    // Initialize nextStates queue with starting state
    // Pop off queue and run TransitionState on dequeued state
    // Continue until empty queue or TransitionState returns true

#endif  // NFA_HPP