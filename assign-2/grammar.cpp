#include <vector>
#include <string>
#include <iostream>
#include "program.cpp"

class fail : std::exception {};

struct Grammar {

std::vector<std::string> tokens;

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
    throw fail();
}

// type_op ::= `)` type_ar
//           | type type_fp
std::pair<Type*, unsigned int> type_op(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "CloseParen") {
        return type_ar(i+1, new Fn());
    }
    auto [t, itemp] = type(i);
    return type_fp(itemp, t);
}

// type_fp ::= `)` type_ar?
//           | (`,` type)+ `)` type_ar
std::pair<Type*, unsigned int> type_fp(unsigned int i, Type* ti) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "CloseParen") {
        try {
            Fn* fn = new Fn();
            fn->prms.push_back(ti);
            return type_ar(i+1, fn);
        } catch (fail& f) {
            return std::make_pair(ti, i+1);
        }
    }
    if (tokens[i] == "Comma") {
        Fn* fn = new Fn();
        fn->prms.push_back(ti);
        auto [t, itemp] = type(i+1);
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
        if (tokens[itemp] == "CloseParen") {
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
    if (i >= tokens.size()) throw fail();
    Fn* fn = new Fn();
    if (tokens[i] == "OpenParen") {
        try {
            auto [t, itemp] = type(i+1);
            fn->prms.push_back(t);
            i = itemp-1;
            while (itemp < tokens.size()) {
                if (tokens[itemp] == "Comma") {
                    auto [another_t, another_itemp] = type(itemp+1);
                    fn->prms.push_back(another_t);
                    itemp = another_itemp;
                    i = itemp-1;
                }
                else throw fail();
            }
        } catch(fail& f) {}
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

/*
int program(int i) {
    
}

int toplevel(int i) {
    
}

int glob(int i) {
    
}

int Typedef(int i) {
    
}

int decl(int i) {
    
}

int decls(int i) {
    
}

int Extern(int i) {
    
}

int fundef(int i) {
    
}

int let(int i) {
    
}

int stmt(int i) {
    
}

int cond(int i) {
    
}

int loop(int i) {
    
}

int block(int i) {
    
}

int assign_or_call(int i) {
    
}

int gets_or_args(int i) {
    
}

int rhs(int i) {
    
}

int lval(int i) {
    
}

int access(int i) {
    
}

int args(int i) {
    
}

int exp(int i) {
    
}

int exp_p4(int i) {
    
}

int exp_p3(int i) {
    
}

int exp_p2(int i) {
    
}

int exp_p1(int i) {
    
}

int exp_ac(int i) {
    
}

int unop(int i) {
    
}

int binop_p1(int i) {
    
}

int binop_p2(int i) {
    
}

int binop_p3(int i) {
    
}*/
};