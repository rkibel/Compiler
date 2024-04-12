#ifndef RE_HPP
#define RE_HPP

#include "nfa.hpp"

#include <utility>

struct RegularExpression {
    std::pair<Node*, Node*> ConvertToNFA(); //a function which returns pair, pointer to first node, then finalNode (final/terminal state). Others will inherit.
    //Don’t define here, just call it virtual function
};

struct Literal : RegularExpression 
{
	CharRange literal; //simple character range in RE
	std::pair<Node*, Node*> ConvertToNFA();
	Literal(CharRange literal) : literal{literal} {};
	Literal(char lower, char upper) : literal(lower, upper) {}
	Literal(char single) : lower(single), upper(single) {};
};

struct Cat : RegularExpression {
	RegularExpression R1;
	RegularExpression R2;
	std::pair<Node*, Node*> ConvertToNFA();
	Cat(RegularExpression R1, RegularExpression R2) : R1(R1), R2(R2) {};
};

struct Union : RegularExpression {
	RegularExpression R1;
	RegularExpression R2;
	std::pair<Node*, Node*> ConvertToNFA();
	Union(RegularExpression R1, RegularExpression R2) : R1(R1), R2(R2) {};
};

struct KleeneStar : RegularExpression {
	RegularExpression inner;
	std::pair<Node*, Node*> ConvertToNFA(); //then define different version
	KleenStar(RegularExpression inner) : inner(inner) {};
};

struct Plus : RegularExpression {
	RegularExpression inner;
	std::pair<Node*, Node*> ConvertToNFA(); //then define different version
	Plus(RegularExpression inner) : inner(inner) {};
};

RegularExpression createStringRE(std::string input);

#endif // RE_HPP