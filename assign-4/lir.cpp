#include <string>
#include <vector>
#include <algorithm>
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

string getMemory(const string& id, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
    auto local_it = find(local_ids.begin(), local_ids.end(), id);
    auto param_it = find(param_ids.begin(), param_ids.end(), id);
    auto function_it = find(function_ids.begin(), function_ids.end(), id);
    if (local_it != local_ids.end()) { //local
        size_t index = distance(local_ids.begin(), local_it);
        return "-" + to_string((index+1)*8) + "(%rbp)";
    } else if (param_it != param_ids.end()) { //parameter
        size_t index = distance(param_ids.begin(), param_it);
        return to_string((index+2)*8) + "(%rbp)"; //+2 because parameters start at 16
    } else if (function_it != function_ids.end()) { //function global
        return id + "_(%rip)";
    } else { // non-function global
        return id + "(%rip)";
    }
}
struct ArithmeticOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const ArithmeticOp& ao) {
        ao.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
        return "";
    }
};
struct Add : ArithmeticOp{
    void print(std::ostream& os) const override { os << "add"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        res += "  movq " + left_mem + ", r8\n";
        res += "  addq " + right_mem + ", r8\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Sub : ArithmeticOp{
    void print(std::ostream& os) const override { os << "sub"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        res += "  movq " + left_mem + ", r8\n";
        res += "  subq " + right_mem + ", r8\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Mul : ArithmeticOp{
    void print(std::ostream& os) const override { os << "mul"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        res += "  movq " + left_mem + ", r8\n";
        res += "  imulq " + right_mem + ", r8\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Div : ArithmeticOp{
    void print(std::ostream& os) const override { os << "div"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        res += "  movq " + left_mem + ", %rax\ncqo\n";
        res += "  movq " + right_mem + ", %r8\n  idivq r8\n";
        res += "  movq %rax, " + lhs_mem + "\n";
        return res;
    }
};

struct ComparisonOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const ComparisonOp& co) {
        co.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
        return "";
    }
};
struct Equal : ComparisonOp{
    void print(std::ostream& os) const override { os << "eq"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        if (left_mem[0] == '$') {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        } else {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        }
        res += "  movq $0, %r8\n  sete %r8b\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct NotEq : ComparisonOp{ 
    void print(std::ostream& os) const override { os << "neq"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        if (left_mem[0] == '$') {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        } else {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        }
        res += "  movq $0, %r8\n  setne %r8b\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Lt : ComparisonOp{
    void print(std::ostream& os) const override { os << "lt"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        if (left_mem[0] == '$') {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        } else {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        }
        res += "  movq $0, %r8\n  setl %r8b\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Lte : ComparisonOp{
    void print(std::ostream& os) const override { os << "lte"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Gt : ComparisonOp{
    void print(std::ostream& os) const override { os << "gt"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        if (left_mem[0] == '$') {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        } else {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        }
        res += "  movq $0, %r8\n  setg %r8b\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Gte : ComparisonOp{
    void print(std::ostream& os) const override { os << "gte"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string res;
        if (left_mem[0] == '$') {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        } else {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        }
        res += "  movq $0, %r8\n  setle %r8b\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};

struct Operand{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const Operand& op) {
        op.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
        return "";
    }
};
struct Const : Operand{
    int32_t num;
    void print(std::ostream& os) const override { os << num; }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Var : Operand{
    VarId id;
    void print(std::ostream& os) const override { os << id; }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};

struct Terminal{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
        t.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
        return "";
    }
};
struct Branch : Terminal{
    Operand* guard;
    BbId tt;
    BbId ff;
    void print(std::ostream& os) const override {
        os << "Branch(" << *guard << ", " << tt << ", " << ff << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct CallDirect : Terminal{
    VarId lhs;
    FuncId callee;
    vector<Operand*> args;
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "CallDirect(" << lhs << ", " << callee << ", [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "], " << next_bb << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct CallIndirect : Terminal{
    VarId lhs;
    VarId callee;
    vector<Operand*> args;
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "CallDirect(" << lhs << ", " << callee << ", [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "], " << next_bb << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Jump : Terminal{
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "Jump(" << next_bb << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Ret : Terminal{
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Ret(" << *op << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
        // "  jmp " + name + "_epilogue\n\n" + 
    }
};


struct LirInst{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const LirInst& li) {
        li.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
        return "";
    }
};
struct Alloc : LirInst{
    VarId lhs;
    Operand* num;
    void print(std::ostream& os) const override {
        os << "Alloc(" << lhs << ", " << *num << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
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
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string lhs_mem, left_mem, right_mem;
        lhs_mem = getMemory(lhs, local_ids, param_ids, function_ids);
        if (Const* c = dynamic_cast<Const*>(left)) {
            left_mem = "$" + c->num;
        } else {
            left_mem = getMemory(static_cast<Var*>(left)->id, local_ids, param_ids, function_ids);
        }
        if (Const* c = dynamic_cast<Const*>(right)) {
            right_mem = "$" + c->num;
        } else {
            right_mem = getMemory(static_cast<Var*>(right)->id, local_ids, param_ids, function_ids);
        }
        return aop->cg(lhs_mem, left_mem, right_mem, local_ids, param_ids, function_ids);
    }
};
struct CallExt : LirInst{
    VarId lhs;
    FuncId callee;
    vector<Operand*> args;
    void print(std::ostream& os) const override {
        os << "CallExt(" << lhs << ", " << callee << ", [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "]" << ")" << endl; 
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
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
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        string lhs_mem, left_mem, right_mem;
        lhs_mem = getMemory(lhs, local_ids, param_ids, function_ids);
        if (Const* c = dynamic_cast<Const*>(left)) {
            left_mem = "$" + c->num;
        } else {
            left_mem = getMemory(static_cast<Var*>(left)->id, local_ids, param_ids, function_ids);
        }
        if (Const* c = dynamic_cast<Const*>(right)) {
            right_mem = "$" + c->num;
        } else {
            right_mem = getMemory(static_cast<Var*>(right)->id, local_ids, param_ids, function_ids);
        }
        return cop->cg(lhs_mem, left_mem, right_mem, local_ids, param_ids, function_ids);
    }
};
struct Copy : LirInst{
    VarId lhs;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Copy(" << lhs << ", " << *op << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        //Five cases for op(rhs): copying a const, local, param, global (non func), global function
        //Three cases for lhs: a local, param, global (non-func)
        //For non-constant, we use a helper function to tell us what string to print
        string res;
        if (Const* c = dynamic_cast<Const*>(op)) { // movq $1, -8(%rbp)
            res += "movq $" + to_string(c->num) + ", " + getMemory(lhs, local_ids, param_ids, function_ids) + "\n";
        } else { //movq -16(%rbp), %r8 /n movq %r8, -8(%rbp)
            res += "movq " + getMemory(static_cast<Var*>(op)->id, local_ids, param_ids, function_ids) + ", r8\n";
            res += "movq %r8, " + getMemory(lhs, local_ids, param_ids, function_ids) + "\n";
        }
        return res;
    }
};
struct Gep : LirInst{
    VarId lhs;
    VarId src;
    Operand* idx;
    void print(std::ostream& os) const override {
        os << "Gep(" << lhs << ", " << src << ", " << *idx << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Gfp : LirInst{
    VarId lhs;
    VarId src;
    string field;
    void print(std::ostream& os) const override {
        os << "Gfp(" << lhs << ", " << src << ", " << field << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Load : LirInst{
    VarId lhs;
    VarId src;
    void print(std::ostream& os) const override {
        os << "Load(" << lhs << ", " << src << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};
struct Store : LirInst{
    VarId dst;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Store(" << dst << ", " << *op << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) override {
        return "";
    }
};

struct BasicBlock{
    BbId label;
    vector<LirInst*> insts;
    Terminal* term;

    friend std::ostream& operator<<(std::ostream& os, const BasicBlock& bb) {
        os << bb.label  << ":" << endl;
        for(unsigned int i = 0; i < bb.insts.size(); i++){
            os << "    " << *bb.insts[i];
        }
        os << "    " << *bb.term;
        return os;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, const vector<string>& function_ids) {
        string res;
        for (LirInst* lir : insts) {
            res += lir->cg(param_ids, local_ids, function_ids);
        }
        res += term->cg(param_ids, local_ids, function_ids);
        return res;
    }
};

template<typename T>
vector<string> getKeys(const std::map<string, T>& myMap) {
    vector<string> keys;
    for (const auto& pair : myMap) {
        keys.push_back(pair.first);
    }
    return keys;
}

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
        for (unsigned int i = 0; i < func.params.size(); i++) {
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

    string cg(vector<string> function_ids) {
        vector<string> param_ids;
            for (const auto& param : params) {
            param_ids.push_back(param.first);
        }
        vector<string> local_ids = getKeys(locals);
        string res;
        res += ".globl " + name + "\n" + name + ":\n  pushq %rbp\n  movq %rsp, %rbp\n";
        int stack_space = (local_ids.size() % 2 == 0) ? local_ids.size() : local_ids.size() + 1;
        res += "  subq $" + to_string(stack_space * 8) + ", %rsp\n";
        for (int i = 1; i < stack_space+1; i++) {
            res += "  movq $0, -" + to_string(i*8) + "(%rbp)\n";
        }
        res += "  jmp " + name + "_entry\n\n" + name + "_entry:\n";
        for (const auto& [bbId, bb]: body) {
            res += bb->cg(local_ids, param_ids, function_ids);
        }
        res += name + "_epilogue:\n  movq %rbp, %rsp\n  popq %rbp\n  ret\n\n";
        return res;
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

    string cg() {
        string res;
        res += ".data\n\n";
        for (const auto& [varid, _]: globals) {
            res += ".globl " + varid + (functions.find(varid) != functions.end()
                ? "_\n" + varid + "_: .quad \"" + varid + "\"\n\n\n"
                : "\n" + varid + ": .zero 8\n\n\n");
        }
        res += "out_of_bounds_msg: .string \"out-of-bounds array access\"\n";
        res += "invalid_alloc_msg: .string \"invalid allocation amount\"\n";
        res += "\n.text\n\n";
        vector<string> function_ids = getKeys(functions);
        for (const auto& [funcId, func] : functions) {
            res += func->cg(function_ids);
        }
       res += ".out_of_bounds:\n  lea out_of_bounds_msg(%rip), %rdi\n  call _cflat_panic\n\n";
       res += ".invalid_alloc_length:\n  lea invalid_alloc_msg(%rip), %rdi\n  call _cflat_panic\n\n";
       return res;
    }    
};


