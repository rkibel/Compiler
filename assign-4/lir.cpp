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
struct BasicBlock;

//Globals
inline vector<string> function_ids;
inline map<string, string> functions_res; //map of function name to the result (res) text (to sort alphabetically)

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

inline string getMemory(const string& id, const vector<string>& local_ids, const vector<string>& param_ids) {
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
    virtual string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) {
        return "";
    }
};
struct Add : ArithmeticOp{
    void print(std::ostream& os) const override { os << "add"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        res += "  movq " + left_mem + ", %r8\n";
        res += "  addq " + right_mem + ", %r8\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Sub : ArithmeticOp{
    void print(std::ostream& os) const override { os << "sub"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        res += "  movq " + left_mem + ", %r8\n";
        res += "  subq " + right_mem + ", %r8\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Mul : ArithmeticOp{
    void print(std::ostream& os) const override { os << "mul"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        res += "  movq " + left_mem + ", %r8\n";
        res += "  imulq " + right_mem + ", %r8\n";
        res += "  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Div : ArithmeticOp{
    void print(std::ostream& os) const override { os << "div"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        res += "  movq " + left_mem + ", %rax\n  cqo\n";
        res += (right_mem[0] == '$') ? "  movq " + right_mem + ", %r8\n  idivq %r8\n" : "  idivq " + right_mem + "\n";
        // res += "  movq " + right_mem + ", %r8\n  idivq %r8\n";
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
    virtual string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) {
        return "";
    }
};
struct Equal : ComparisonOp{
    void print(std::ostream& os) const override { os << "eq"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  sete %r8b\n  movq %r8, -8(%rbp)\n";
        return res;
    }
};
struct NotEq : ComparisonOp{ 
    void print(std::ostream& os) const override { os << "neq"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setne %r8b\n  movq %r8, -8(%rbp)\n";
        return res;
    }
};
struct Lt : ComparisonOp{
    void print(std::ostream& os) const override { os << "lt"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setl %r8b\n  movq %r8, -8(%rbp)\n";
        return res;
    }
};
struct Lte : ComparisonOp{
    void print(std::ostream& os) const override { os << "lte"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setle %r8b\n  movq %r8, -8(%rbp)\n";
        return res;
    }
};
struct Gt : ComparisonOp{
    void print(std::ostream& os) const override { os << "gt"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setg %r8b\n  movq %r8, -8(%rbp)\n";
        return res;
    }
};
struct Gte : ComparisonOp{
    void print(std::ostream& os) const override { os << "gte"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setge %r8b\n  movq %r8, -8(%rbp)\n";
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
    virtual string cg(const vector<string>& local_ids, const vector<string>& param_ids) {
        return "";
    }
};
struct Const : Operand{
    int32_t num;
    void print(std::ostream& os) const override { os << num; }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
        return "";
    }
};
struct Var : Operand{
    VarId id;
    void print(std::ostream& os) const override { os << id; }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
        return "";
    }
};

inline string getMemoryOperand(Operand*& op, const vector<string>& local_ids, const vector<string>& param_ids) {
    if (Const* c = dynamic_cast<Const*>(op)) {
        return "$" + to_string(c->num);
    } else {
        return getMemory(static_cast<Var*>(op)->id, local_ids, param_ids);
    }
}

struct Terminal{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
        t.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) {
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) override;
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) override;
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) override;
};
struct Jump : Terminal{
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "Jump(" << next_bb << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) override;
};
struct Ret : Terminal{
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Ret(" << *op << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) override {
        string res;
        res += "  movq " + getMemoryOperand(op, local_ids, param_ids) + ", %rax\n  jmp " + func_name + "_epilogue\n\n";
        return res;
    }
};


struct LirInst{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const LirInst& li) {
        li.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const vector<string>& local_ids, const vector<string>& param_ids) {
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
        string lhs_mem = getMemory(lhs, local_ids, param_ids);
        string left_mem = getMemoryOperand(left, local_ids, param_ids);
        string right_mem = getMemoryOperand(right, local_ids, param_ids);
        return aop->cg(lhs_mem, left_mem, right_mem, local_ids, param_ids);
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
        string res;
        vector<string> six_args{"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
        int numPushed = 0;
        for (unsigned int i = 0; i < args.size(); i++) {
            if (i < 6) { 
                res += "  movq " + getMemoryOperand(args[i], local_ids, param_ids) + ", " + six_args[i] + "\n";
            } else {
                res += "  pushq " +  getMemoryOperand(args[args.size()-1-(i-6)], local_ids, param_ids) + "\n";
                numPushed++;
            }
        }
        if ( args.size() > 6 && args.size() % 2 != 0 ) { res += "  subq $8, %rsp\n"; numPushed++; }
        res += "  call " + callee + "\n";
        if (numPushed > 0) { res += "  addq $" + to_string(numPushed*8) +", %rsp\n"; }
        if (lhs != "_") {
            res += "  movq %rax, " + getMemory(lhs, local_ids, param_ids) + "\n";
        }
        return res;
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
        // cout << "COMPARISON\n";
        string lhs_mem = getMemory(lhs, local_ids, param_ids);
        string left_mem = getMemoryOperand(left, local_ids, param_ids);
        string right_mem = getMemoryOperand(right, local_ids, param_ids);
        return cop->cg(lhs_mem, left_mem, right_mem, local_ids, param_ids);
    }
};
struct Copy : LirInst{
    VarId lhs;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Copy(" << lhs << ", " << *op << ")" << endl;
    }
    //TO-DO
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
        //Five cases for op(rhs): copying a const, local, param, global (non func), global function
        //Three cases for lhs: a local, param, global (non-func)
        //For non-constant, we use a helper function to tell us what string to print
        string op_mem = getMemoryOperand(op, local_ids, param_ids);
        string res = "  movq " + op_mem + ", ";
        res += (op_mem[0] == '$') ? getMemory(lhs, local_ids, param_ids) + "\n" : "%r8\n  movq %r8, " + getMemory(lhs, local_ids, param_ids) + "\n";
        // if (Const* c = dynamic_cast<Const*>(op)) { // movq $1, -8(%rbp)
        //     res += "  movq $" + to_string(c->num) + ", " + getMemory(lhs, local_ids, param_ids) + "\n";
        // } else { //movq -16(%rbp), %r8 /n   movq %r8, -8(%rbp)
        //     res += "  movq " + getMemory(static_cast<Var*>(op)->id, local_ids, param_ids) + ", %r8\n";
        //     res += "  movq %r8, " + getMemory(lhs, local_ids, param_ids) + "\n";
        // }
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
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
    string cg(const vector<string>& local_ids, const vector<string>& param_ids) override {
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
    void cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) {
        string res = func_name + "_" + label + ":\n";
        for (LirInst* lir : insts) {
            res += lir->cg(local_ids, param_ids);
        }
        res += term->cg(local_ids, param_ids, blocks, func_name, labels_map);
        labels_map[label] = res;
    }
};

template<typename T>
void getKeys(const map<string, T>& myMap, vector<string>& keys) {
    for (const auto& pair : myMap) {
        keys.push_back(pair.first);
    }
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

    void cg() {
        vector<string> param_ids;
            for (const auto& param : params) {
            param_ids.push_back(param.first);
        }
        vector<string> local_ids; 
        getKeys(locals, local_ids);
        string res;
        res += ".globl " + name + "\n" + name + ":\n  pushq %rbp\n  movq %rsp, %rbp\n";
        int stack_space = (local_ids.size() % 2 == 0) ? local_ids.size() : local_ids.size() + 1;
        res += "  subq $" + to_string(stack_space * 8) + ", %rsp\n";
        for (unsigned int i = 1; i < local_ids.size() + 1; i++) {
            res += "  movq $0, -" + to_string(i*8) + "(%rbp)\n";
        }
        res += "  jmp " + name + "_entry\n\n";
        map<string, string> labels_map;
        for (const auto& [bbId, bb]: body) {
            bb->cg(local_ids, param_ids, body, name, labels_map);
        }
        for (const auto& [label_name, label_res] : labels_map) {
            res += label_res;
        }
        res += name + "_epilogue:\n  movq %rbp, %rsp\n  popq %rbp\n  ret\n\n";
        functions_res[name] = res;
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
        getKeys(functions, function_ids);
        for (const auto& [funcId, func] : functions) {
            func->cg();
        }
        for (const auto& [func_name, func_res] : functions_res) {
            res += func_res;
        }
        res += ".out_of_bounds:\n  lea out_of_bounds_msg(%rip), %rdi\n  call _cflat_panic\n\n";
        res += ".invalid_alloc_length:\n  lea invalid_alloc_msg(%rip), %rdi\n  call _cflat_panic\n\n";
        return res;
    }    
};


//cg functions which rely on later definitions
inline string Jump::cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) {
    string res = "  $jmp " + func_name + "_" + next_bb + "\n\n";
    blocks[next_bb]->cg(local_ids, param_ids, blocks, func_name, labels_map);
    return res;
}

inline string Branch::cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) {
    string res;
    if (Const* c = dynamic_cast<Const*>(guard)) {
        res += "  movq " + to_string(c->num) + ", %r8\n  cmpq $0, %r8\n";
    } else {
        res += "  cmpq $0, " + getMemory(dynamic_cast<Var*>(guard)->id, local_ids, param_ids) + "\n";
    }
    res += "  jne " + func_name + "_" + tt + "\n  jmp " + func_name + "_" + ff + "\n\n";
    blocks[tt]->cg(local_ids, param_ids, blocks, func_name, labels_map);
    blocks[ff]->cg(local_ids, param_ids, blocks, func_name, labels_map);
    return res;
}

inline string CallDirect::cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) {
    string res;
    int numPushed = 0;
    if ( args.size() % 2 != 0 ) { res += "  subq $8, %rsp\n"; numPushed++; }
    for (int i = args.size() - 1; i >= 0; i--) {
        res += "  pushq " + getMemoryOperand(args[i], local_ids, param_ids) + "\n";
        numPushed++;
    }
    res += "  call " + callee + "\n";
    if (lhs != "_") { res += "  movq %rax, " + getMemory(lhs, local_ids, param_ids) + "\n"; }
    if (numPushed > 0) { res += "  addq $" + to_string(numPushed*8) +", %rsp\n"; }
    res += "  jmp " + func_name + "_" + next_bb + "\n\n";
    blocks[next_bb]->cg(local_ids, param_ids, blocks, func_name, labels_map);
    return res;
}

inline string CallIndirect::cg(const vector<string>& local_ids, const vector<string>& param_ids, map<BbId, BasicBlock*>& blocks, const string& func_name, map<string, string>& labels_map) {
    string res;
    int numPushed = 0;
    if ( args.size() % 2 != 0 ) { res += "  subq $8, %rsp\n"; numPushed++; }
    for (int i = args.size() - 1; i >= 0; i--) {
        res += "  pushq " + getMemoryOperand(args[i], local_ids, param_ids) + "\n";
        numPushed++;
    }
    res += "  call *" + getMemory(callee, local_ids, param_ids) + "\n";
    if (lhs != "_") { res += "  movq %rax, " + getMemory(lhs, local_ids, param_ids) + "\n"; }
    if (numPushed > 0) { res += "  addq $" + to_string(numPushed*8) +", %rsp\n"; }
    res += "  jmp " + func_name + "_" + next_bb + "\n\n";
    blocks[next_bb]->cg(local_ids, param_ids, blocks, func_name, labels_map);
    return res;
}

