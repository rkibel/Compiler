#include <vector>
#include <string>
#include <iostream>
#include "program.cpp"

class fail : std::exception {};

struct Grammar {

std::vector<std::string> tokens;

//Notice how all type returns std::pair<Type*, unsigned int>, recursive calls until this is always returned: the current AST struct type and then the next index into the token vector
//Then binop rules will return std::pair<BinaryOp*, unsigned int>, this behaves similarly

// type ::= `&`* type_ad
std::pair<Type*, unsigned int> type(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Address") {
        Ptr* ptr = new Ptr();
        auto [ref, inext] = type(i+1);
        ptr->ref = ref;
        return std::make_pair(ptr, inext);
    }
    return type_ad(i);
}

// type_ad ::= `int`
//           | id
//           | `(` type_op
std::pair<Type*, unsigned int> type_ad(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Int") {
        return std::make_pair(new Int(), i+1);
    }
    if (tokens[i].substr(0, 2) == "Id") {
        StructType* st = new StructType();
        st->name = tokens[i].substr(3, tokens[i].size()-4);
        return std::make_pair(st, i+1);
    }
    if (tokens[i] == "OpenParen") {
        return type_op(i+1);
    }
    throw fail(); //if type not int, id, or function
}

// type_op ::= `)` type_ar
//           | type type_fp
std::pair<Type*, unsigned int> type_op(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "CloseParen") {
        return type_ar(i+1, new Fn()); //New function with no parameters
    }
    auto [t, itemp] = type(i);
    return type_fp(itemp, t);
}

// type_fp ::= `)` type_ar?
//           | (`,` type)+ `)` type_ar
// Extra parameter used to keep track of the last parameter passed in
std::pair<Type*, unsigned int> type_fp(unsigned int i, Type* ti) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "CloseParen") {
        try {
            Fn* fn = new Fn(); //Creates new function with exactly one parameter (no comma)
            fn->prms.push_back(ti);
            return type_ar(i+1, fn);
        } catch (fail& f) { //Not real fail, just if no arrows in this case (type_ar)
            return std::make_pair(ti, i+1);
        }
    }
    if (tokens[i] == "Comma") {
        Fn* fn = new Fn(); //Creates function with at least two parameters
        fn->prms.push_back(ti);
        auto [t, itemp] = type(i+1); //itemp will store next token to look at
        fn->prms.push_back(t);
        try {
            while (itemp < tokens.size()) {
                if (tokens[itemp] == "Comma") {
                    auto [new_t, new_itemp] = type(itemp+1);
                    fn->prms.push_back(new_t);
                    itemp = new_itemp;
                }
                else throw fail();
            }
        } catch (fail& f) {}
        if (tokens[itemp] == "CloseParen") { //If type comes back not as int, id, or func (type_ad)
            return type_ar(itemp+1, fn);    
        }
    }
    throw fail();
}

// type_ar ::= `->` rettyp
std::pair<Type*, unsigned int> type_ar(unsigned int i, Fn* fn) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Arrow") {
        auto [t, itemp] = rettyp(i+1);
        fn->ret = t;
        return std::make_pair(fn, itemp);
    }
    throw fail();
}

// funtype ::= `(` (type (`,` type)*)? `)` `->` rettyp
std::pair<Type*, unsigned int> funtype(unsigned int i) noexcept(false) {
    //Note: maybe the i redeclaration "i = itemp-1;" isn't needed. Maybe can just use itemp?
    if (i >= tokens.size()) throw fail();
    Fn* fn = new Fn();
    if (tokens[i] == "OpenParen") {
        try {
            auto [t, itemp] = type(i+1);
            fn->prms.push_back(t);
            i = itemp-1; //index of the last type
            while (itemp < tokens.size()) {
                if (tokens[itemp] == "Comma") {
                    auto [another_t, another_itemp] = type(itemp+1); //returns index after type to check
                    fn->prms.push_back(another_t);
                    itemp = another_itemp;
                    i = itemp-1;
                }
                else throw fail(); //No more arguments
            }
        } catch(fail& f) {} //No more arguments, close paren then arrow
        if (tokens[i+1] == "CloseParen" && tokens[i+2] == "Arrow") {
            auto [t, itemp] = rettyp(i+3);
            fn->ret = t;
            return std::make_pair(fn, itemp);
        }
    }
    throw fail();
}

// rettyp ::= type 
//           | `_`
std::pair<Type*, unsigned int> rettyp(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    try {
        return type(i);
    } catch (fail& f) {}
    if (tokens[i] == "Underscore") {
        return std::make_pair(new Any(), i+1);
    }
    throw fail();
}

// unop ::= `*`
//        | `-`
std::pair<UnaryOp*, unsigned int> unop(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Star") return std::make_pair(new UnaryDeref(), i+1);
    if (tokens[i] == "Dash") return std::make_pair(new Neg(), i+1);
    throw fail();
}

// binop_p1 ::= `*` | `/`
std::pair<BinaryOp*, unsigned int> binop_p1(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Star") return std::make_pair(new Mul(), i+1);
    if (tokens[i] == "Slash") return std::make_pair(new Div(), i+1);
    throw fail();
}

// binop_p2 ::= `+` | `-`
std::pair<BinaryOp*, unsigned int> binop_p2(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Plus") return std::make_pair(new Add(), i+1);
    if (tokens[i] == "Dash") return std::make_pair(new Sub(), i+1);
    throw fail();
}

// binop_p3 ::= `==` | `!=` | `<` | `<=` | `>` | `>=`
std::pair<BinaryOp*, unsigned int> binop_p3(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Equal") return std::make_pair(new Equal(), i+1);
    if (tokens[i] == "NotEq") return std::make_pair(new NotEq(), i+1);
    if (tokens[i] == "Lt") return std::make_pair(new Lt(), i+1);
    if (tokens[i] == "Lte") return std::make_pair(new Lte(), i+1);
    if (tokens[i] == "Gt") return std::make_pair(new Gt(), i+1);
    if (tokens[i] == "Gte") return std::make_pair(new Gte(), i+1);
    throw fail();
}

// TODO: implement exp, exp_p4, exp_p3, exp_p2, exp_p1, and exp_ac

// exp ::= exp_p4 (binop_p3 exp_p4)*

std::pair<Exp*, unsigned int> exp(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    try { 
        auto [lref, inext] = exp_p4(i+1);
    } catch (fail& f) { //First part isn't exp_p4
        throw fail();
    }

    //lref will chagne
    try {
        while (itemp < tokens.size()) { //keep looping for 0 or more binop_p3 exp_p4
            auto [opref, itemp] = binop_p3(inext);
            inext = itemp;
            try { //successful operation, must be a successful exp_p4 to be valid
                auto [rref, another_itemp] = exp_p4(inext);
                inext = another_itemp;
            } catch (fail& f) { //binop_3 but no exp_p4, fails. Not valid
                throw fail();
            }
            BinOp* binop = new Binop(); //Can create a new binop
            binop.left = lref;
            binop.op = opref;
            binop.right = rref;
            lref = binop;
        }
    } catch(fail& f) {
        return std::make_pair(lref, inext)
    }
}



// exp_p4 ::= exp_p3 (binop_p2 exp_p3)*
// exp_p3 ::= exp_p2 (binop_p1 exp_p2)*
// exp_p2 ::= unop* exp_p1
// exp_p1 ::= num
//          | `nil`
//          | `(` exp `)`
//          | id exp_ac*
// exp_ac ::= `[` exp `]`
//          | `.` id
//          | `(` args? `)`

};