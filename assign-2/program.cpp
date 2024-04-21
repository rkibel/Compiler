#include <string>
#include <vector>
#include <optional>
#include <iostream>

struct Type {
    virtual void print(std::ostream& os) const {
        os << "Hello";
    }
    virtual ~Type() {}
};
struct Int : Type {
    void print(std::ostream& os) const override {
        os << "Int";
    }
};
struct StructType : Type {
    std::string name;
    void print(std::ostream& os) const override { 
        os << "Struct(" << name << ")"; 
    }
};
struct Fn : Type {
    std::vector<Type*> prms;
    Type* ret;
    void print(std::ostream& os) const override { 
        os << "Fn(prms = [";
        for (unsigned int i = 0; i < prms.size(); i++) {
            prms[i]->print(os);
            if (i != prms.size() - 1) os << ", ";
        }
        os << "], ret = ";
        ret->print(os);
        os << ")";
    }
    ~Fn() {
        for (Type* t: prms) delete t;
        delete ret;
    }

};
struct Ptr : Type {
    Type* ref;
    void print(std::ostream& os) const override { 
        os << "Ptr(";
        ref->print(os);
        os << ")";
    }
    ~Ptr() { delete ref; }
};
struct Any : Type {
    void print(std::ostream& os) const override { 
        os << "_";
    }
};
std::ostream& operator<<(std::ostream& os, const Type& obj) {
    obj.print(os);
    return os;
}

/*
struct UnaryOp {};
struct Neg : UnaryOp {};
struct Deref : UnaryOp {};

struct BinaryOp{};
struct Add : BinaryOp{};
struct Sub : BinaryOp{};
struct Mul : BinaryOp{};
struct Div : BinaryOp{};
struct Equal : BinaryOp{};
struct NotEq : BinaryOp{};
struct Lt : BinaryOp{};
struct Lte : BinaryOp{};
struct Gt : BinaryOp{};
struct Gte : BinaryOp{};

struct Decl {
    std::string name;
    Type type;
};

struct Struct {
    std::string name;
    std::vector<Decl> fields;
};

struct Exp {};
struct Num : Exp {
    int32_t n;
};
struct ExpId : Exp {
    std::string name;
};
struct Nil : Exp {};
struct UnOp : Exp {
    UnaryOp op;
    Exp operand;
};
struct BinOp : Exp {
    BinaryOp op;
    Exp left;
    Exp right;
};
struct ArrayAccess : Exp {
    Exp ptr;
    Exp index;
};
struct FieldAccess : Exp {
    Exp ptr;
    std::string field;
};
struct Call : Exp {
    Exp callee;
    std::vector<Exp> args;
};

struct Lval {};
struct LvalId : Lval {
    std::string name;
};
struct Deref : Lval {
    Lval lval;
};
struct ArrayAccess : Lval {
    Lval ptr;
    Exp index;
};
struct FieldAccess : Lval {
    Lval ptr;
    std::string field;
};

struct Stmt {};
struct Break : Stmt {};
struct Continue : Stmt {};
struct Return : Stmt {
    std::optional<Exp> exp;
};
struct Assign : Stmt {
    Lval lhs;
    Rhs rhs;
};
struct Call : Stmt {
    Lval callee;
    std::vector<Exp> args;
};
struct If : Stmt {
    Exp guard;
    std::vector<Stmt> tt;
    std::vector<Stmt> ff;
};
struct While : Stmt {
    Exp guard;
    std::vector<Stmt> body;
};

struct Function {
    std::string name;
    std::vector<Decl> params;
    std::optional<Type> rettyp;
    std::vector<std::pair<Decl, std::optional<Exp>>> locals;
    std::vector<Stmt> stmts;
};

struct Rhs {};
struct RhsExp : Rhs {
    Exp exp;
};
struct New : Rhs {
    Type type;
    Exp amount;
};


struct Program {
    std::vector<Decl> globals;
    std::vector<Struct> structs;
    std::vector<Decl> externs;
    std::vector<Function> functions;
};*/