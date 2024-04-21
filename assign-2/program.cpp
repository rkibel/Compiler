#include <string>
#include <vector>
#include <optional>
#include <iostream>

struct Type {
    virtual void print(std::ostream& os) const {}
    virtual ~Type() {}
    friend std::ostream& operator<<(std::ostream& os, const Type& obj) {
        obj.print(os);
        return os;
    }
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
            os << *prms[i];
            if (i != prms.size() - 1) os << ", ";
        }
        os << "], ret = " << *ret << ")";
    }
    ~Fn() {
        for (Type* t: prms) delete t;
        delete ret;
    }

};
struct Ptr : Type {
    Type* ref;
    void print(std::ostream& os) const override { 
        os << "Ptr(" << *ref << ")";
    }
    ~Ptr() { delete ref; }
};
struct Any : Type {
    void print(std::ostream& os) const override { 
        os << "_";
    }
};

struct UnaryOp {
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const UnaryOp& uo) {
        uo.print(os);
        return os;
    }
};
struct Neg : UnaryOp {
    void print(std::ostream& os) const override { os << "Neg"; }
};
struct UnaryDeref : UnaryOp {
    void print(std::ostream& os) const override { os << "Deref"; }
};

struct BinaryOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const BinaryOp& bo) {
        bo.print(os);
        return os;
    }
};
struct Add : BinaryOp{
    void print(std::ostream& os) const override { os << "Add"; }
};
struct Sub : BinaryOp{
    void print(std::ostream& os) const override { os << "Sub"; }
};
struct Mul : BinaryOp{
    void print(std::ostream& os) const override { os << "Mul"; }
};
struct Div : BinaryOp{
    void print(std::ostream& os) const override { os << "Div"; }
};
struct Equal : BinaryOp{
    void print(std::ostream& os) const override { os << "Equal"; }
};
struct NotEq : BinaryOp{
    void print(std::ostream& os) const override { os << "NotEq"; }
};
struct Lt : BinaryOp{
    void print(std::ostream& os) const override { os << "Lt"; }
};
struct Lte : BinaryOp{
    void print(std::ostream& os) const override { os << "Lte"; }
};
struct Gt : BinaryOp{
    void print(std::ostream& os) const override { os << "Gt"; }
};
struct Gte : BinaryOp{
    void print(std::ostream& os) const override { os << "Gte"; }
};

// TODO
struct Decl {
    std::string name;
    Type type;
};

// TODO
struct Struct {
    std::string name;
    std::vector<Decl> fields;
};

struct Exp {
    virtual void print(std::ostream& os) const {}
    virtual ~Exp() {}
    friend std::ostream& operator<<(std::ostream& os, const Exp& exp) {
        exp.print(os);
        return os;
    }
};
struct Num : Exp {
    int32_t n;
    void print(std::ostream& os) const override { os << "Num(" << n << ")"; }
};
struct ExpId : Exp {
    std::string name;
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
};
struct Nil : Exp {
    void print(std::ostream& os) const override { os << "Nil"; }
};
struct UnOp : Exp {
    UnaryOp* op;
    Exp* operand;
    void print(std::ostream& os) const override { os << *op << "(" << *operand << ")"; }
    ~UnOp() { delete operand; }
};
struct BinOp : Exp {
    BinaryOp* op;
    Exp* left;
    Exp* right;
    void print(std::ostream& os) const override {
        os << "BinOp(\nop = " << *op << ",\nleft = " << *left << ",\nright = " << *right << "\n)"; 
    }
    ~BinOp() { delete left; delete right; }
};
struct ExpArrayAccess : Exp {
    Exp* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    ~ExpArrayAccess() { delete ptr; delete index; }
};
struct ExpFieldAccess : Exp {
    Exp* ptr;
    std::string field;
    void print(std::ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    ~ExpFieldAccess() { delete ptr; }
};
struct ExpCall : Exp {
    Exp* callee;
    std::vector<Exp*> args;
    void print(std::ostream& os) const override {
        os << "Call(\ncallee = " << *callee << ",\nargs = [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *args[i];
            if (i != args.size() - 1) os << ",";
            os << "\n]\n)";
        }
    }
    ~ExpCall() {
        delete callee;
        for (Exp* exp: args) delete exp;
    }
};

// TODO EVERYTHING BELOW THIS
struct Lval {};
struct LvalId : Lval {
    std::string name;
};
struct LvalDeref : Lval {
    Lval lval;
};
struct LvalArrayAccess : Lval {
    Lval ptr;
    Exp index;
};
struct LvalFieldAccess : Lval {
    Lval ptr;
    std::string field;
};

struct Rhs {};
struct RhsExp : Rhs {
    Exp exp;
};
struct New : Rhs {
    Type type;
    Exp amount;
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
struct StmtCall : Stmt {
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

struct Program {
    std::vector<Decl> globals;
    std::vector<Struct> structs;
    std::vector<Decl> externs;
    std::vector<Function> functions;
};