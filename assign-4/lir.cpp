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
        os << "Fn([";
        for (unsigned int i = 0; i < prms.size(); i++) {
            os << *prms[i];
            if (i != prms.size() - 1) os << ", ";
        }
        os << "], " << *ret << ")";
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
    void print(std::ostream& os) const override { os << "add"; }
};
struct Sub : ArithmeticOp{
    void print(std::ostream& os) const override { os << "sub"; }
};
struct Mul : ArithmeticOp{
    void print(std::ostream& os) const override { os << "mul"; }
};
struct Div : ArithmeticOp{
    void print(std::ostream& os) const override { os << "div"; }
};

struct ComparisonOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const ComparisonOp& co) {
        co.print(os);
        return os;
    }
};
struct Equal : ComparisonOp{
    void print(std::ostream& os) const override { os << "eq"; }
};
struct NotEq : ComparisonOp{ 
    void print(std::ostream& os) const override { os << "neq"; }
};
struct Lt : ComparisonOp{
    void print(std::ostream& os) const override { os << "lt"; }
};
struct Lte : ComparisonOp{
    void print(std::ostream& os) const override { os << "lte"; }
};
struct Gt : ComparisonOp{
    void print(std::ostream& os) const override { os << "gt"; }
};
struct Gte : ComparisonOp{
    void print(std::ostream& os) const override { os << "gte"; }
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
        os << "Branch(" << *guard << ", " << tt << ", " << ff << ")" << endl;
    }
};
struct CallDirect : Terminal{
    VarId lhs;
    FuncId callee;
    vector<Operand*> args;
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "CallDirect(" << lhs << ", " << callee << ", [";
        for (int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "], " << next_bb << ")" << endl;
    }
};
struct CallIndirect : Terminal{
    VarId lhs;
    VarId callee;
    vector<Operand*> args;
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "CallIndirect(" << lhs << ", " << callee << ", [";
        for (int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "], " << next_bb << ")" << endl;
    }
};
struct Jump : Terminal{
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "Jump(" << next_bb << ")" << endl;
    }
};
struct Ret : Terminal{
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Ret(" << *op << ")" << endl;
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
        os << "Alloc(" << lhs << ", " << *num << ")" << endl;
    }
};
struct Arith : LirInst{
    VarId lhs;
    ArithmeticOp* aop;
    Operand* left;
    Operand* right;
    void print(std::ostream& os) const override {
        os << "Arith(" << lhs << ", " << *aop << ", " << *left << ", " << *right << ")" << endl;
    }
};
struct CallExt : LirInst{
    VarId lhs;
    FuncId callee;
    vector<Operand*> args;
    void print(std::ostream& os) const override {
        os << "CallExt(" << lhs << ", " << callee << ", [";
        for (int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "]" << ")" << endl; 
    }
};
struct Cmp : LirInst{
    VarId lhs;
    ComparisonOp* cop;
    Operand* left;
    Operand* right;
    void print(std::ostream& os) const override {
        os << "Cmp(" << lhs << ", " << *cop << ", " << *left << ", " << *right << ")" << endl;
    }
};
struct Copy : LirInst{
    VarId lhs;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Copy(" << lhs << ", " << *op << ")" << endl;
    }
};
struct Gep : LirInst{
    VarId lhs;
    VarId src;
    Operand* idx;
    void print(std::ostream& os) const override {
        os << "Gep(" << lhs << ", " << src << ", " << *idx << ")" << endl;
    }
};
struct Gfp : LirInst{
    VarId lhs;
    VarId src;
    string field;
    void print(std::ostream& os) const override {
        os << "Gfp(" << lhs << ", " << src << ", " << field << ")" << endl;
    }
};
struct Load : LirInst{
    VarId lhs;
    VarId src;
    void print(std::ostream& os) const override {
        os << "Load(" << lhs << ", " << src << ")" << endl;
    }
};
struct Store : LirInst{
    VarId dst;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Store(" << dst << ", " << *op << ")" << endl;
    }
};

struct BasicBlock{
    BbId label;
    vector<LirInst*> insts;
    Terminal* term;

    friend std::ostream& operator<<(std::ostream& os, const BasicBlock& bb) {
        os << bb.label  << ":" << endl;
        for(int i = 0; i < bb.insts.size(); i++){
            os << "    " << *bb.insts[i];
        }
        os << "    " << *bb.term;
        return os;
    }
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
        os << "Function " << func.name << "(";
        for (int i = 0; i < func.params.size(); i++) {
            os << func.params[i].first << ":" << *func.params[i].second;
            if (i != func.params.size() - 1) os << ", ";
        }
        os << ") -> " << *func.rettyp << " {" << endl;
        os << "  Locals" << endl;
        for (auto local : func.locals) {
            os << "    ";
            os << local.first << " : " << *local.second << endl;
        }
        os << endl;
    
        for (auto it = func.body.begin(); it != func.body.end(); ++it) {
            os << "  " << *it->second;
            if (std::next(it) != func.body.end()) {
                os << endl;
            }
        }
        os << "}" << endl;
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
        for(auto s: prog.structs){
            os << "Struct " << s.first << endl;
            for(auto field : s.second){
                os << "  " << field.first << " : " << *field.second << endl;
            }
            os << endl;
        }
        
        os << "Externs" << endl;
        for(auto e: prog.externs){
            os << "  " << e.first << " : " << *e.second << endl;
        }
        os << endl;

        os << "Globals" << endl;
        for(auto g: prog.globals){
            os << "  " << g.first << " : " << *g.second << endl;
        }
        os << endl;

        for(auto f: prog.functions){
            os << *f.second << endl;
        }
        
        return os;
    }
};


