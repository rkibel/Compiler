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
        if (itemp < tokens.size() && tokens[itemp] == "CloseParen") { //If type comes back not as int, id, or func (type_ad)
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
        if (i+2 < tokens.size() && tokens[i+1] == "CloseParen" && tokens[i+2] == "Arrow") {
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
    auto [lref, inext] = exp_p4(i);
    BinOp* binop = nullptr;
    try {
        while (inext < tokens.size()) {
            auto [opref, itemp] = binop_p3(inext);
            auto [rref, another_itemp] = exp_p4(inext+1);
            inext = another_itemp;

            if (!binop) {
                binop = new BinOp();
                binop->op = opref;
                binop->left = lref;
                binop->right = rref;    
            }
            else {
                BinOp* binop2 = new BinOp();
                binop2->op = opref;
                binop2->left = binop;
                binop2->right = rref;
                binop = binop2;
            }
        }
    } catch(fail& f) {}
    if (!binop) return std::make_pair(lref, inext);
    return std::make_pair(binop, inext);
}

// exp_p4 ::= exp_p3 (binop_p2 exp_p3)*
std::pair<Exp*, unsigned int> exp_p4(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    auto [lref, inext] = exp_p3(i);
    BinOp* binop = nullptr;
    try {
        while (inext < tokens.size()) {
            auto [opref, itemp] = binop_p2(inext);
            auto [rref, another_itemp] = exp_p3(itemp);
            inext = another_itemp;

            if (!binop) {
                binop = new BinOp();
                binop->op = opref;
                binop->left = lref;
                binop->right = rref;    
            }
            else {
                BinOp* binop2 = new BinOp();
                binop2->op = opref;
                binop2->left = binop;
                binop2->right = rref;
                binop = binop2;
            }
        }
    } catch(fail& f) {}
    if (!binop) return std::make_pair(lref, inext);
    return std::make_pair(binop, inext);
}

// exp_p3 ::= exp_p2 (binop_p1 exp_p2)*
std::pair<Exp*, unsigned int> exp_p3(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    auto [lref, inext] = exp_p2(i);
    BinOp* binop = nullptr;
    try {
        while (inext < tokens.size()) {
            auto [opref, itemp] = binop_p1(inext);
            auto [rref, another_itemp] = exp_p2(itemp);
            inext = another_itemp;

            if (!binop) {
                binop = new BinOp();
                binop->op = opref;
                binop->left = lref;
                binop->right = rref;    
            }
            else {
                BinOp* binop2 = new BinOp();
                binop2->op = opref;
                binop2->left = binop;
                binop2->right = rref;
                binop = binop2;
            }
        }
    } catch(fail& f) {}
    if (!binop) return std::make_pair(lref, inext);
    return std::make_pair(binop, inext);
}

// exp_p2 ::= unop* exp_p1
std::pair<Exp*, unsigned int> exp_p2(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    try {
        auto [op, itemp] = unop(i);
        auto [operand, inext] = exp_p2(itemp);
        UnOp* unop = new UnOp();
        unop->op = op;
        unop->operand = operand;
        return std::make_pair(unop, inext);
    } catch(fail& f) {}
    return exp_p1(i);
}

// exp_p1 ::= num
//          | `nil`
//          | `(` exp `)`
//          | id exp_ac*
std::pair<Exp*, unsigned int> exp_p1(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i].substr(0, 3) == "Num") {
        Num* num = new Num();
        try {
            num->n = std::stoi(tokens[i].substr(4, tokens[i].size() - 5));
        } catch(std::exception& e) { 
            // Not sure what to do here, if Num(x) has x as an invalid number then we enter this catch
            throw fail();
        }
        return std::make_pair(num, i+1);
    }
    else if (tokens[i] == "Nil") { return std::make_pair(new Nil(), i+1); }
    else if (tokens[i] == "OpenParen") {
        auto [e, itemp] = exp(i+1);
        if (itemp < tokens.size() && tokens[itemp] == "CloseParen") { return std::make_pair(e, itemp+1); }
        throw fail();
    }
    else if (tokens[i].substr(0, 2) == "Id") {
        ExpId* expid = new ExpId();
        expid->name = tokens[i].substr(3, tokens[i].size() - 4); 
        Exp* ret = expid;
        unsigned int itemp = i+1;
        try {
            while (itemp < tokens.size()) {
                auto [e, inext] = exp_ac(itemp, ret);
                ret = e;
                itemp = inext;
            }
        } catch(fail& f) {}
        return std::make_pair(ret, itemp);
    }
    throw fail();

}

// exp_ac ::= `[` exp `]`
//          | `.` id
//          | `(` args? `)`
std::pair<Exp*, unsigned int> exp_ac(unsigned int i, Exp* e) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "OpenBracket") {
        auto [another_e, itemp] = exp(i+1);
        if (itemp < tokens.size() && tokens[itemp] == "CloseBracket") {
            ExpArrayAccess* eaa = new ExpArrayAccess();
            eaa->ptr = e;
            eaa->index = another_e;
            return std::make_pair(eaa, itemp+1);
        }
    }
    else if (tokens[i] == "Dot") {
        if (i+1 < tokens.size() && tokens[i+1].substr(0, 2) == "Id") {
            ExpFieldAccess* efa = new ExpFieldAccess();
            efa->ptr = e;
            efa->field = tokens[i+1].substr(3, tokens[i+1].size()-4);
            return std::make_pair(efa, i+2);
        }
    }
    else if (tokens[i] == "OpenParen") {
        ExpCall* ec = new ExpCall();
        ec -> callee = e;
        try {
            auto [list_e, itemp] = args(i+1);
            if (itemp < tokens.size() && tokens[itemp] == "CloseParen") {
                ec->args = list_e;
                return std::make_pair(ec, itemp+1);
            }
        } catch (fail& f){}
        if (i+1 < tokens.size() && tokens[i+1] == "CloseParen") return std::make_pair(ec, i+2);
    }
    throw fail();
}

// args ::= exp (`,` exp)*
std::pair<std::vector<Exp*>, unsigned int> args(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    std::vector<Exp*> vec;
    auto [e, itemp] = exp(i);
    vec.push_back(e);
    try {
        while (itemp < tokens.size()) {
            if (tokens[itemp] == "Comma") {
                auto [another_e, inext] = exp(itemp+1);
                vec.push_back(another_e);
                itemp = inext;
            }
            else throw fail();
        }
    } catch (fail& f) {}
    return std::make_pair(vec, itemp);
}

// lval ::= `*`* id access*
std::pair<Lval*, unsigned int> lval(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Star") {
        auto [lval1, itemp] = lval(i+1);
        LvalDeref* lvd = new LvalDeref();
        lvd->lval = lval1;
        return std::make_pair(lvd, itemp);
    }
    if (tokens[i].substr(0, 2) == "Id") {
        LvalId* lvalid = new LvalId();
        lvalid->name = tokens[i].substr(3, tokens[i].size() - 4);
        Lval* ret = lvalid;
        unsigned int itemp = i+1;
        try {
            while (itemp < tokens.size()) {
                auto [lv, inext] = access(itemp, ret);
                ret = lv;
                itemp = inext;
            }
        } catch (fail& f) {}
        return std::make_pair(ret, itemp);
    }
    throw fail();
}

// access ::= `[` exp `]` 
//          | `.` id
std::pair<Lval*, unsigned int> access(unsigned int i, Lval* lv) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "OpenBracket") {
        auto [e, itemp] = exp(i+1);
        if (itemp < tokens.size() && tokens[itemp] == "CloseBracket") {
            LvalArrayAccess* laa = new LvalArrayAccess();
            laa->ptr = lv;
            laa->index = e;
            return std::make_pair(laa, itemp+1);
        }
    }
    else if (tokens[i] == "Dot") {
        if (i+1 < tokens.size() && tokens[i+1].substr(0, 2) == "Id") {
            LvalFieldAccess* lfa = new LvalFieldAccess();
            lfa->ptr = lv;
            lfa->field = tokens[i+1].substr(3, tokens[i+1].size()-4);
            return std::make_pair(lfa, i+2);
        }
    }
    throw fail();
}

// assign_or_call ::= lval gets_or_args
std::pair<Stmt*, unsigned int> assign_or_call(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    auto [lv, itemp] = lval(i);
    return gets_or_args(itemp, lv);
}

// gets_or_args ::= `=` rhs
//                | `(` args? `)`
std::pair<Stmt*, unsigned int> gets_or_args(unsigned int i, Lval* lv) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Gets") {
        auto [r, itemp] = rhs(i+1);
        Assign* assign = new Assign();
        assign->lhs = lv;
        assign->rhs = r;
        return std::make_pair(assign, itemp);
    }
    else if (tokens[i] == "OpenParen") {
        StmtCall* stmtcall = new StmtCall();
        stmtcall->callee = lv;
        try {
            auto [exps, itemp] = args(i+1);
            if (itemp < tokens.size() && tokens[itemp] == "CloseParen") {
                stmtcall->args = exps;
                return std::make_pair(stmtcall, itemp+1);
            }
        } catch (fail& f) {}
        if (i + 1 < tokens.size() && tokens[i+1] == "CloseParen") return std::make_pair(stmtcall, i+2);
    }
    throw fail();
}

// rhs ::= exp 
//       | `new` type exp?
std::pair<Rhs*, unsigned int> rhs(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    try {
        auto [e, itemp] = exp(i);
        RhsExp* rhse = new RhsExp();
        rhse->exp = e;
        return std::make_pair(rhse, itemp);
    } catch (fail& f) {}
    if (tokens[i] == "New") {
        auto [t, itemp1] = type(i+1);
        // too many news
        New* n = new New();
        n->type = t;
        try {
            auto [e, itemp2] = exp(itemp1);
            n->amount = e;
            return std::make_pair(n, itemp2);
        } catch (fail& f) {}
        // if no exp is in place for new, create Num(1) as exp
        Num* num = new Num();
        num->n = 1;
        n->amount = num;
        return std::make_pair(n, itemp1);
    }
    throw fail();
}

// decl ::= id `:` type
std::pair<Decl*, unsigned int> decl(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (i+1 < tokens.size() && tokens[i].substr(0, 2) == "Id" && tokens[i+1] == "Colon") {
        auto [t, itemp] = type(i+2);
        Decl* d = new Decl();
        d->name = tokens[i].substr(3, tokens[i].size()-4);
        d->type = t;
        return std::make_pair(d, itemp);
    }
    throw fail();
}

// decls ::= decl (`,` decl)*
std::pair<std::vector<Decl*>, unsigned int> decls(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    std::vector<Decl*> vec;
    auto [d, itemp] = decl(i);
    vec.push_back(d);
    try {
        while (itemp < tokens.size()) {
            if (tokens[itemp] == "Comma") {
                auto [another_d, inext] = decl(itemp+1);
                vec.push_back(another_d);
                itemp = inext;
            }
            else throw fail();
        }
    } catch (fail& f) {}
    return std::make_pair(vec, itemp);
}

// extern ::= `extern` id `:` funtype `;`
std::pair<Decl*, unsigned int> exter(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (i + 2 < tokens.size() && tokens[i] == "Extern" && tokens[i+1].substr(0, 2) == "Id" && tokens[i+2] == "Colon") {
        auto [t, itemp] = funtype(i+3);
        if (itemp < tokens.size() && tokens[itemp] == "Semicolon") {
            Decl* dec = new Decl();
            dec->name = tokens[i+1].substr(3, tokens[i+1].size() - 4);
            dec->type = t;
            return std::make_pair(dec, itemp+1);
        }
    }
    throw fail();
}

// fundef ::= `fn` id `(` decls? `)` `->` rettyp `{` let* stmt+ `}`
std::pair<Function*, unsigned int> fundef(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (i + 2 < tokens.size() && tokens[i] == "Fn" && tokens[i+1].substr(0, 2) == "Id" && tokens[i+2] == "OpenParen") {
        Function* func = new Function();
        func->name = tokens[i+1].substr(3, tokens[i+1].size()-4);
        unsigned int itemp = i+3;
        try {
            auto [dec, itemp1] = decls(itemp);
            func->params = dec;
            itemp = itemp1;
        } catch (fail& f) {}
        if (itemp + 1 < tokens.size() && tokens[itemp] == "CloseParen" && tokens[itemp+1] == "Arrow") {
            auto [ret, itemp1] = rettyp(itemp+2);
            func->rettyp = ret;
            if (itemp1 < tokens.size() && tokens[itemp1] == "OpenBrace") {
                itemp1++;
                try {
                    while (itemp1 < tokens.size()) {
                        auto [vec, inext] = let(itemp1);
                        for (auto pair: vec) func->locals.push_back(pair);
                        itemp1 = inext;
                    }
                } catch (fail& f) {}
                auto [s, itemp2] = stmt(itemp1);
                itemp1 = itemp2;
                func->stmts.push_back(s);
                try {
                    while (itemp1 < tokens.size()) {
                        auto [another_s, inext] = stmt(itemp1);
                        func->stmts.push_back(another_s);
                        itemp1 = inext;
                    }
                } catch (fail& f) {}
                if (itemp1 < tokens.size() && tokens[itemp1] == "CloseBrace") return std::make_pair(func, itemp1+1);
            }
        }
    }
    throw fail();
}

// let ::= `let` decl (`=` exp)? (`,` decl (`=` exp)?)* `;`

// rewriting this to the following:
// let ::= `let` intermediate_let (`,` intermediate_let)* `;`
std::pair<std::vector<std::pair<Decl*, Exp*>>, unsigned int> let(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Let") {
        std::vector<std::pair<Decl*, Exp*>> vec;
        auto [pair, itemp] = intermediate_let(i+1);
        vec.push_back(pair);
        try {
            while (itemp < tokens.size()) {
                if (tokens[itemp] == "Comma") {
                    auto [another_pair, inext] = intermediate_let(itemp+1);
                    vec.push_back(another_pair);
                    itemp = inext;
                }
                else throw fail();
            }
        } catch (fail& f) {}
        if (itemp < tokens.size() && tokens[itemp] == "Semicolon") return std::make_pair(vec, itemp+1);
    }
    throw fail();
}

// intermediate_let ::= decl (`=` exp)?
std::pair<std::pair<Decl*, Exp*>, unsigned int> intermediate_let(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    auto [d, itemp] = decl(i);
    try {
        if (itemp < tokens.size() && tokens[itemp] == "Gets") {
            auto [e, itemp1] = exp(itemp+1);
            std::pair<Decl*, Exp*> p = std::make_pair(d, e);
            return std::make_pair(p, itemp1);
        }
    } catch (fail& f) {}
    std::pair<Decl*, Exp*> p = std::make_pair(d, new AnyExp());
    return std::make_pair(p, itemp);
}

// stmt ::= cond               # conditional
//        | loop               # loop
//        | assign_or_call `;` # assignment or function call
//        | `break` `;`        # break out of a loop
//        | `continue` `;`     # continue to next iteration of loop
//        | `return` exp? `;`  # return from function
std::pair<Stmt*, unsigned int> stmt(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    try {
        return cond(i);
    } catch (fail& f) {}
    try {
        return loop(i);
    } catch (fail& f) {}
    try {
        auto [res, itemp] = assign_or_call(i);
        if (itemp < tokens.size() && tokens[itemp] == "Semicolon") return std::make_pair(res, itemp+1);
        else throw fail();
    } catch (fail& f) {}
    if (i + 1 < tokens.size() && tokens[i] == "Break" && tokens[i+1] == "Semicolon") return std::make_pair(new Break(), i+2);
    if (i + 1 < tokens.size() && tokens[i] == "Continue" && tokens[i+1] == "Semicolon") return std::make_pair(new Continue(), i+2);
    if (tokens[i] == "Return") {
        try {
            auto [e, itemp] = exp(i+1);
            if (itemp < tokens.size() && tokens[itemp] == "Semicolon") {
                Return* ret = new Return();
                ret->exp = e;
                return std::make_pair(ret, itemp+1);
            }
        } catch (fail& f) {}
        if (i+1 < tokens.size() && tokens[i+1] == "Semicolon") {
            Return* ret = new Return();
            AnyExp* any = new AnyExp();
            ret->exp = any;
            return std::make_pair(ret, i+2);
        }
    }
    throw fail();
}


// cond ::= `if` exp block (`else` block)?
std::pair<Stmt*, unsigned int> cond(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "If") {
        auto [e, itemp] = exp(i+1);
        auto [vec, itemp1] = block(itemp);
        If* ifs = new If();
        ifs->guard = e;
        ifs->tt = vec;
        try {
            if (itemp1 < tokens.size() && tokens[itemp1] == "Else") {
                auto [another_vec, itemp2] = block(itemp1+1);
                ifs->ff = another_vec;
                itemp1 = itemp2;
            }
        } catch (fail& f) {}
        return std::make_pair(ifs, itemp1);
    }
    throw fail();
}

// loop ::= `while` exp block 
std::pair<Stmt*, unsigned int> loop(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "While") {
        auto [e, itemp] = exp(i+1);
        auto [vec, itemp1] = block(itemp);
        While* wh = new While();
        wh->guard = e;
        wh->body = vec;
        return std::make_pair(wh, itemp1);
    }
    throw fail();
}

// block ::= `{` stmt* `}`
std::pair<std::vector<Stmt*>, unsigned int> block(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "OpenBrace") {
        std::vector<Stmt*> vec;
        unsigned int itemp = i+1;
        try {
            while (itemp < tokens.size()) {
                auto [s, inext] = stmt(itemp);
                vec.push_back(s);
                itemp = inext;
            }
        } catch (fail& f) {}
        if (itemp < tokens.size() && tokens[itemp] == "CloseBrace") return std::make_pair(vec, itemp+1);
    }
    throw fail();
}

//  program ::= toplevel+
Program* program(unsigned int i) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    Program* prog = new Program();
    unsigned int itemp = toplevel(i, prog);
    try {
        while (itemp < tokens.size()) {
            itemp = toplevel(itemp, prog);
        }
    } catch (fail& f) {}
    if (itemp == tokens.size()) return prog;
    throw fail();
}

// toplevel ::= glob      # global variable declaration
//            | typedef   # struct type definition
//            | extern    # external function declaration
//            | fundef    # function definition
unsigned int toplevel(unsigned int i, Program* prog) noexcept(false) {
    if (i >= tokens.size()) throw fail();
    try {
        auto [vec_globals, itemp] = glob(i);
        for (auto global: vec_globals) prog->globals.push_back(global);
        return itemp;
    } catch (fail& f) {}
    try {
        auto [str, itemp] = typedefn(i);
        prog->structs.push_back(str);
        return itemp;
    } catch (fail& f) {}
    try {
        auto [dec, itemp] = exter(i);
        prog->externs.push_back(dec);
        return itemp;
    } catch (fail& f) {}
    auto [function, itemp] = fundef(i);
    prog->functions.push_back(function);
    return itemp;
}

// # global variable declaration.
// glob ::= `let` decls `;`
std::pair<std::vector<Decl*>, unsigned int> glob(unsigned int i) {
    if (i >= tokens.size()) throw fail();
    if (tokens[i] == "Let") {
        auto [vec, itemp] = decls(i+1);
        if (itemp < tokens.size() && tokens[itemp] == "Semicolon") return std::make_pair(vec, itemp+1);
    }
    throw fail();
}

// # struct type definition.
// typdef ::= `struct` id `{` decls `}`
std::pair<Struct*, unsigned int> typedefn(unsigned int i) {
    if (i >= tokens.size()) throw fail();
    if (i+2 < tokens.size() && tokens[i] == "Struct" && tokens[i+1].substr(0, 2) == "Id" && tokens[i+2] == "OpenBrace") {
        auto [vec, itemp] = decls(i+3);
        if (itemp < tokens.size() && tokens[itemp] == "CloseBrace") {
            Struct* str = new Struct();
            str->name = tokens[i+1].substr(3, tokens[i+1].size()-4);
            str->fields = vec;
            return std::make_pair(str, itemp+1);
        }
    }
    throw fail();
}

};