#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>

using namespace std;

using StructId = string;
using FuncId = string;
using VarId = string;
using FieldId = string;
using BbId = string;

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
    // WARNING: if there is no return for Fn, ret is Any (look at struct Any : Type)
    Type* ret;
    void print(std::ostream& os) const override { 
        os << "Fn(prms = [";
        for (unsigned int i = 0; i < prms.size(); i++) {
            os << *prms[i];
            if (i != prms.size() - 1) os << ", ";
        }
        os << "], ret = " << *ret << ")";
    }
};
struct Ptr : Type {
    Type* ref;
    void print(std::ostream& os) const override { 
        os << "Ptr(" << *ref << ")";
    }
};
struct Any : Type {
    void print(std::ostream& os) const override { 
        os << "_";
    }
};

struct ArithmeticOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const ArithmeticOp& ao) {
        ao.print(os);
        return os;
    }
};
struct Add : ArithmeticOp{
    void print(std::ostream& os) const override { os << "Add"; }
};
struct Sub : ArithmeticOp{
    void print(std::ostream& os) const override { os << "Sub"; }
};
struct Mul : ArithmeticOp{
    void print(std::ostream& os) const override { os << "Mul"; }
};
struct Div : ArithmeticOp{
    void print(std::ostream& os) const override { os << "Div"; }
};

struct ComparisonOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const ComparisonOp& co) {
        co.print(os);
        return os;
    }
};
struct Equal : ComparisonOp{
    void print(std::ostream& os) const override { os << "Equal"; }
};
struct NotEq : ComparisonOp{ 
    void print(std::ostream& os) const override { os << "NotEq"; }
};
struct Lt : ComparisonOp{
    void print(std::ostream& os) const override { os << "Lt"; }
};
struct Lte : ComparisonOp{
    void print(std::ostream& os) const override { os << "Lte"; }
};
struct Gt : ComparisonOp{
    void print(std::ostream& os) const override { os << "Gt"; }
};
struct Gte : ComparisonOp{
    void print(std::ostream& os) const override { os << "Gte"; }
};

struct Operand{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const Operand& op) {
        op.print(os);
        return os;
    }
};
struct Const : Operand{
    int32_t num;
    void print(std::ostream& os) const override { os << num; }
};
struct Var : Operand{
    VarId id;
    void print(std::ostream& os) const override { os << id; }
};

struct Terminal{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
        t.print(os);
        return os;
    }
};
struct Branch : Terminal{
    Operand* guard;
    BbId tt;
    BbId ff;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};

struct CallDirect : Terminal{
    VarId lhs;
    FuncId callee;
    vector<Operand*> args;
    BbId next_bb;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct CallIndirect : Terminal{
    VarId lhs;
    VarId callee;
    vector<Operand*> args;
    BbId next_bb;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Jump : Terminal{
    BbId next_bb;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Ret : Terminal{
    Operand* op;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};


struct LirInst{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const LirInst& li) {
        li.print(os);
        return os;
    }
};
struct Alloc : LirInst{
    VarId lhs;
    Operand* num;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Arith : LirInst{
    VarId lhs;
    ArithmeticOp* aop;
    Operand* left;
    Operand* right;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct CallExt : LirInst{
    VarId lhs;
    FuncId callee;
    vector<Operand*> args;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Cmp : LirInst{
    VarId lhs;
    ComparisonOp* aop;
    Operand* left;
    Operand* right;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Copy : LirInst{
    VarId lhs;
    Operand* op;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Gep : LirInst{
    VarId lhs;
    VarId src;
    Operand* idx;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Gfp : LirInst{
    VarId lhs;
    VarId src;
    string field;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Load : LirInst{
    VarId lhs;
    VarId src;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};
struct Store : LirInst{
    VarId dst;
    Operand* op;
    void print(std::ostream& os) const override {
        // TODO : override print 
    }
};

struct BasicBlock{
    BbId label;
    vector<LirInst*> insts;
    Terminal* term;
};

struct Function{
    FuncId name;
    vector<pair<VarId, Type*>> params;
    Type* rettyp;
    map<VarId, Type*> locals;
    map<BbId, BasicBlock*> body;

    ~Function() { 
    }
    friend std::ostream& operator<<(std::ostream& os, const Function& func) {
        os << "Sample Function";
        return os;
    }
};

struct Program{
    map<VarId, Type*> globals;    
    map<StructId, map<FieldId, Type*>>structs;
    map<FuncId, Type*> externs;
    map<FuncId, Function*> functions;

    ~Program() { 
    }

    friend std::ostream& operator<<(std::ostream& os, const Program& prog) {
        os << "Externs" << endl;
        for(auto e: prog.externs){
            os << e.first << " : " << e.second << endl;
        }
        os << endl;

        os << "Globals" << endl;
        for(auto g: prog.globals){
            os << g.first << " : " << g.second << endl;
        }
        os << endl;

        for(auto f: prog.functions){
            os << "Function " << f.second << endl;
            os << endl;
        }
        return os;
    }
};


