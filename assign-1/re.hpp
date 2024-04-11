#ifndef RE_HPP
#define RE_HPP

#include "nfa.hpp"

#include <utility>

struct CharRange {
    char lower;
    char upper;
};

struct RegularExpression {
    std::pair<Node*, Node*> ConvertToNFA(); //a function which returns pair, pointer to first node, then finalNode (final/terminal state). Others will inherit.
    //Don’t define here, just call it virtual function
};

struct LiteralExpression : RegularExpression 
{
	CharRange literal; //simple character range in RE
	std::pair<Node*, Node*> ConvertToNFA();
	
};

struct Cat : RegularExpression {
	RegularExpression R1;
	RegularExpression R2;
	std::pair<Node*, Node*> ConvertToNFA();
};

struct Union : RegularExpression {
	RegularExpression R1;
	RegularExpression R2;
	std::pair<Node*, Node*> ConvertToNFA();
};

struct KleenStar : RegularExpression {
	RegularExpression inner;
	std::pair<Node*, Node*> ConvertToNFA(); //then define different version
};

struct Plus : RegularExpression {
	RegularExpression inner;
	std::pair<Node*, Node*> ConvertToNFA(); //then define different version
};

#endif // RE_HPP