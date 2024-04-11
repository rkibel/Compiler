#include "re.hpp"
#include "nfa.hpp"

LiteralExpression :: ConvertToNFA {
    Node startNode;
    startNode.isTerminalNode = false;
    startNode.transitions.push_back
}