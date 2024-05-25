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
struct Function;
struct BasicBlock;

//Globals
inline vector<string> function_ids;
inline vector<string> param_ids; //for whichever functino you're currently on
inline vector<string> local_ids; //''
inline map<string, string> functions_res; //map of function name to the result (res) text (to sort alphabetically)
inline vector<string> visited_labels;

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

//Two global pointers so we don't need to pass in a pointer to the program to every function
inline map<StructId, map<FieldId, Type*>>* global_structs; //a reference to the program's struct map
inline map<VarId, Type*>* global_globals; //haha

template<typename T>
void getKeys(const map<string, T>& input_map, vector<string>& output_vector) {
    for (const auto& pair : input_map) {
        output_vector.push_back(pair.first);
    }
}

struct ArithmeticOp{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const ArithmeticOp& ao) {
        ao.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) { return ""; }
};
struct Add : ArithmeticOp{
    void print(std::ostream& os) const override { os << "add"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
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
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
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
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
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
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
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
    virtual string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) { return ""; }
};
struct Equal : ComparisonOp{
    void print(std::ostream& os) const override { os << "eq"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  sete %r8b\n  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct NotEq : ComparisonOp{ 
    void print(std::ostream& os) const override { os << "neq"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setne %r8b\n  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Lt : ComparisonOp{
    void print(std::ostream& os) const override { os << "lt"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setl %r8b\n  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Lte : ComparisonOp{
    void print(std::ostream& os) const override { os << "lte"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setle %r8b\n  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Gt : ComparisonOp{
    void print(std::ostream& os) const override { os << "gt"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setg %r8b\n  movq %r8, " + lhs_mem + "\n";
        return res;
    }
};
struct Gte : ComparisonOp{
    void print(std::ostream& os) const override { os << "gte"; }
    //TO-DO
    string cg(const string& lhs_mem, const string& left_mem, const string& right_mem, Function*& f) override {
        string res;
        if (right_mem[0] == '$' && left_mem[0] != '$') {
            res += "  cmpq " + right_mem + ", " + left_mem + "\n";
        } else {
            res += "  movq " + left_mem + ", %r8\n";
            res += "  cmpq " + right_mem + ", %r8\n";
        }
        res += "  movq $0, %r8\n  setge %r8b\n  movq %r8, " + lhs_mem + "\n";
        return res;
    }
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
    //TO-DO
    virtual string cg(Function*& f, map<string, string>& labels_map) { return ""; }
};
struct Branch : Terminal{
    Operand* guard;
    BbId tt;
    BbId ff;
    void print(std::ostream& os) const override {
        os << "Branch(" << *guard << ", " << tt << ", " << ff << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f, map<string, string>& labels_map) override;
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
    string cg(Function*& f, map<string, string>& labels_map) override;
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
    string cg(Function*& f, map<string, string>& labels_map) override;
};
struct Jump : Terminal{
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "Jump(" << next_bb << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f, map<string, string>& labels_map) override;
};
struct Ret : Terminal{
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Ret(" << *op << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f, map<string, string>& labels_map) override;
};


struct LirInst{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const LirInst& li) {
        li.print(os);
        return os;
    }
    //TO-DO
    virtual string cg(Function*& f) { return ""; }
};
struct Alloc : LirInst{
    VarId lhs;
    Operand* num;
    void print(std::ostream& os) const override {
        os << "Alloc(" << lhs << ", " << *num << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f) override;
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
    string cg(Function*& f) override;
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
    string cg(Function*& f) override;
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
    string cg(Function*& f) override;
};
struct Copy : LirInst{
    VarId lhs;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Copy(" << lhs << ", " << *op << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f) override;
};
struct Gep : LirInst{
    VarId lhs;
    VarId src;
    Operand* idx;
    void print(std::ostream& os) const override {
        os << "Gep(" << lhs << ", " << src << ", " << *idx << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f);
};
struct Gfp : LirInst{
    VarId lhs;
    VarId src;
    string field;
    void print(std::ostream& os) const override {
        os << "Gfp(" << lhs << ", " << src << ", " << field << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f) override;
};
struct Load : LirInst{
    VarId lhs;
    VarId src;
    void print(std::ostream& os) const override {
        os << "Load(" << lhs << ", " << src << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f) override;
};
struct Store : LirInst{
    VarId dst;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Store(" << dst << ", " << *op << ")" << endl;
    }
    //TO-DO
    string cg(Function*& f) override;
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
    void cg(Function*& f, map<string, string>& labels_map);
};

struct Function{
    FuncId name;
    vector<pair<VarId, Type*>> params;
    Type* rettyp;
    map<VarId, Type*> locals;
    map<BbId, BasicBlock*> body;

    ~Function() { 
    }
    friend std::ostream& operator<<(std::ostream& os, Function& func) {
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

    void cg(Function*& f) {
        param_ids.clear();
        for (const auto& param : f->params) {
        param_ids.push_back(param.first);
        }
        local_ids.clear();
        getKeys(f->locals, local_ids);

        string res;
        res += ".globl " + name + "\n" + name + ":\n  pushq %rbp\n  movq %rsp, %rbp\n";
        int stack_space = (f->locals.size() % 2 == 0) ? f->locals.size() : f->locals.size() + 1;
        res += "  subq $" + to_string(stack_space * 8) + ", %rsp\n";
        for (unsigned int i = 1; i < f->locals.size() + 1; i++) {
            res += "  movq $0, -" + to_string(i*8) + "(%rbp)\n";
        }
        res += "  jmp " + name + "_entry\n\n";
        map<string, string> labels_map;
        for (const auto& [bbId, bb]: body) {
            bb->cg(f, labels_map);
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
        global_structs = &structs;
        global_globals = &globals;
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
        for (auto& [funcId, f] : functions) {
            f->cg(f);
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
inline string getMemory(const string& id, Function*& f) {
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
inline string getMemoryOperand(Operand*& op, Function*& f) {
    if (Const* c = dynamic_cast<Const*>(op)) {
        return "$" + to_string(c->num);
    } else {
        return getMemory(static_cast<Var*>(op)->id, f);
    }
}

inline string Arith::cg(Function*& f) {
        string lhs_mem = getMemory(lhs, f);
        string left_mem = getMemoryOperand(left, f);
        string right_mem = getMemoryOperand(right, f);
        return aop->cg(lhs_mem, left_mem, right_mem, f);
    }

inline string Cmp::cg(Function*& f) {
    string lhs_mem = getMemory(lhs, f);
    string left_mem = getMemoryOperand(left, f);
    string right_mem = getMemoryOperand(right, f);
    return cop->cg(lhs_mem, left_mem, right_mem, f);
}

inline string Copy::cg(Function*& f) {
    //Five cases for op(rhs, f): copying a const, local, param, global (non func), global function
    //Three cases for lhs: a local, param, global (non-func)
    //For non-constant, we use a helper function to tell us what string to print
    string op_mem = getMemoryOperand(op, f);
    string res = "  movq " + op_mem + ", ";
    res += (op_mem[0] == '$') ? getMemory(lhs, f) + "\n" : "%r8\n  movq %r8, " + getMemory(lhs, f) + "\n";
    // if (Const* c = dynamic_cast<Const*>(op)) { // movq $1, -8(%rbp)
    //     res += "  movq $" + to_string(c->num) + ", " + getMemory(lhs, f) + "\n";
    // } else { //movq -16(%rbp), %r8 /n   movq %r8, -8(%rbp)
    //     res += "  movq " + getMemory(static_cast<Var*>(op)->id) + ", %r8\n";
    //     res += "  movq %r8, " + getMemory(lhs, f) + "\n";
    // }
    return res;
}

inline string Ret::cg(Function*& f, map<string, string>& labels_map) {
    string res;
    res += "  movq " + getMemoryOperand(op, f) + ", %rax\n  jmp " + f->name + "_epilogue\n\n";
    return res;
}

inline string Load::cg(Function*& f) {
    string res;
    res += "  movq " + getMemory(src, f) + ", %r8\n  movq 0(%r8), %r9\n";
    res += "  movq %r9, " + getMemory(lhs, f) + "\n";
    return res;
}

inline string Store::cg(Function*& f) {
    string res;
    res += "  movq " + getMemoryOperand(op, f) + ", %r8\n";
    res += "  movq " + getMemory(dst, f) + ", %r9\n  movq %r8, 0(%r9)\n";
    return res;
}

inline void BasicBlock::cg(Function*& f, map<string, string>& labels_map) {
    string res = f->name + "_" + label + ":\n";
    for (LirInst* lir : insts) {
        res += lir->cg(f);
    }
    res += term->cg(f, labels_map);
    labels_map[label] = res;
}

inline string Jump::cg(Function*& f, map<string, string>& labels_map) {
    string res = "  jmp " + f->name + "_" + next_bb + "\n\n";
    if (find(visited_labels.begin(), visited_labels.end(), next_bb) == visited_labels.end()) {
        visited_labels.push_back(next_bb);
        f->body[next_bb]->cg(f, labels_map);
    }
    return res;
}

inline string Branch::cg(Function*& f, map<string, string>& labels_map) {
    string res;
    if (Const* c = dynamic_cast<Const*>(guard)) {
        res += "  movq $" + to_string(c->num) + ", %r8\n  cmpq $0, %r8\n";
    } else {
        res += "  cmpq $0, " + getMemory(static_cast<Var*>(guard)->id, f) + "\n";
    }
    res += "  jne " + f->name + "_" + tt + "\n  jmp " + f->name + "_" + ff + "\n\n";
    if (find(visited_labels.begin(), visited_labels.end(), tt) == visited_labels.end()) {
        visited_labels.push_back(tt);
        f->body[tt]->cg(f, labels_map);
    }
    if (find(visited_labels.begin(), visited_labels.end(), ff) == visited_labels.end()) {
        visited_labels.push_back(ff);
        f->body[ff]->cg(f, labels_map);
    }
    return res;
}

inline string CallDirect::cg(Function*& f, map<string, string>& labels_map) {
    string res;
    int numPushed = 0;
    if ( args.size() % 2 != 0 ) { res += "  subq $8, %rsp\n"; numPushed++; }
    for (int i = args.size() - 1; i >= 0; i--) {
        res += "  pushq " + getMemoryOperand(args[i], f) + "\n";
        numPushed++;
    }
    res += "  call " + callee + "\n";
    if (lhs != "_") { res += "  movq %rax, " + getMemory(lhs, f) + "\n"; }
    if (numPushed > 0) { res += "  addq $" + to_string(numPushed*8) +", %rsp\n"; }
    res += "  jmp " + f->name + "_" + next_bb + "\n\n";
    if (find(visited_labels.begin(), visited_labels.end(), next_bb) == visited_labels.end()) {
        visited_labels.push_back(next_bb);
        f->body[next_bb]->cg(f, labels_map);
    }
    return res;
}

inline string CallIndirect::cg(Function*& f, map<string, string>& labels_map) {
    string res;
    int numPushed = 0;
    if ( args.size() % 2 != 0 ) { res += "  subq $8, %rsp\n"; numPushed++; }
    for (int i = args.size() - 1; i >= 0; i--) {
        res += "  pushq " + getMemoryOperand(args[i], f) + "\n";
        numPushed++;
    }
    res += "  call *" + getMemory(callee, f) + "\n";
    if (lhs != "_") { res += "  movq %rax, " + getMemory(lhs, f) + "\n"; }
    if (numPushed > 0) { res += "  addq $" + to_string(numPushed*8) +", %rsp\n"; }
    res += "  jmp " + f->name + "_" + next_bb + "\n\n";
    if (find(visited_labels.begin(), visited_labels.end(), next_bb) == visited_labels.end()) {
        visited_labels.push_back(next_bb);
        f->body[next_bb]->cg(f, labels_map);
    }
    return res;
}

inline string CallExt::cg(Function*& f) {
    string res;
    vector<string> six_args{"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    int numPushed = 0;
    for (unsigned int i = 0; i < args.size(); i++) {
        if (i < 6) { 
            res += "  movq " + getMemoryOperand(args[i], f) + ", " + six_args[i] + "\n";
        } else {
            res += "  pushq " +  getMemoryOperand(args[args.size()-1-(i-6)], f) + "\n";
            numPushed++;
        }
    }
    if ( args.size() > 6 && args.size() % 2 != 0 ) { res += "  subq $8, %rsp\n"; numPushed++; }
    res += "  call " + callee + "\n";
    if (lhs != "_") {
        res += "  movq %rax, " + getMemory(lhs, f) + "\n";
    }
    if (numPushed > 0) { res += "  addq $" + to_string(numPushed*8) +", %rsp\n"; }
    return res;
}

inline int getStructNumWords(StructId s_id, Function*& f) {
    //returns 1 if not a struct type, otherwise returns how many words it contains (since they must at least contain one member)
    auto local_it = f->locals.find(s_id);
    auto param_it = std::find_if(f->params.begin(), f->params.end(),
        [s_id](const std::pair<VarId, Type*>& element) {
            return element.first == s_id;
    });
    auto global_it = global_globals->find(s_id);
    if (local_it != f->locals.end()) { //local
        if (Ptr* ptr = dynamic_cast<Ptr*>(local_it->second)) {
            if (StructType* str = dynamic_cast<StructType*>(ptr->ref)) { return (*global_structs)[str->name].size(); } else { return 1; }
        } else { return 1; } //if an integer type
    } else if (param_it != f->params.end()) { //param
        if (Ptr* ptr = dynamic_cast<Ptr*>(param_it->second)) {
            if (StructType* str = dynamic_cast<StructType*>(ptr->ref)) { return (*global_structs)[str->name].size(); } else { return 1; }
        } else { return 1; } //if an integer type
    } else { //global
        if (Ptr* ptr = dynamic_cast<Ptr*>(global_it->second)) {
            if (StructType* str = dynamic_cast<StructType*>(ptr->ref)) { return (*global_structs)[str->name].size(); } else { return 1; }
        } else { return 1; } //if an integer type
    }
    return -1; //error condition
}

inline string Alloc::cg(Function*& f) {
        string res;
        bool was_const = false;
        if (Const* c = dynamic_cast<Const*>(num)) {
            res += "  movq $" + to_string(c->num) + ", %r8\n  cmpq $0, %r8\n";
            was_const = true;
        } else {
            res += "  cmpq $0, " + getMemory(static_cast<Var*>(num)->id, f) + "\n";
        }
        res += "  jle .invalid_alloc_length\n";
        //update for struct
        // if (f->)
        res += "  movq $" + to_string(getStructNumWords(lhs, f)) + ", %rdi\n";
        res += "  imulq " + ( (was_const) ? "%r8" : getMemoryOperand(num, f) ) + ", %rdi\n  incq %rdi\n  call _cflat_alloc\n";
        res += "  movq " + getMemoryOperand(num, f) + ", %r8\n  movq %r8, 0(%rax)\n  addq $8, %rax\n";
        res += "  movq %rax, " + getMemory(lhs, f) + "\n";
        return res;
    }

inline int getStructFieldIndex(StructId s_id, FieldId f_id, Function*& f) {
    //guarenteed to point to a correct struct name
    //returns index of field
    Ptr* struct_ptr;
    auto local_it = f->locals.find(s_id);
    auto param_it = std::find_if(f->params.begin(), f->params.end(),
        [s_id](const std::pair<VarId, Type*>& element) {
            return element.first == s_id;
    });
    auto global_it = global_globals->find(s_id);
    if (local_it != f->locals.end()) { //local
        struct_ptr = static_cast<Ptr*>(local_it->second);
        
    } else if (param_it != f->params.end()) { //param
        struct_ptr = static_cast<Ptr*>(param_it->second);
    } else { //global
        struct_ptr = static_cast<Ptr*>(global_it->second);
    }
    StructType* str = static_cast<StructType*>(struct_ptr->ref);
    auto fields = (*global_structs)[str->name]; 
    return distance(fields.begin(), fields.find(f_id));
}

inline string Gfp::cg(Function*& f) {
    string res;
    res += "  movq " + getMemory(src, f) + ", %r8\n";
    res += "  leaq " + to_string(8 * getStructFieldIndex(src, field, f)) + "(%r8), %r9\n";
    res += "  movq %r9, " + getMemory(lhs, f) + "\n";
    return res;
}

inline string Gep::cg(Function*& f) {
    string res;
    res += "  movq " + getMemoryOperand(idx, f) + ", %r8\n  cmpq $0, %r8\n  jl .out_of_bounds\n";
    res += "  movq " + getMemory(src, f) + ", %r9\n  movq -8(%r9), %r10\n  cmpq %r10, %r8\n  jge .out_of_bounds\n";
    res += "  imulq $" + to_string(8 * getStructNumWords(src, f)) + ", %r8\n  addq %r9, %r8\n";
    res += "  movq %r8, " + getMemory(lhs, f) + "\n";
    return res;
}
