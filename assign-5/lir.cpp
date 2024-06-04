#include <string>
#include <vector>
#include <algorithm>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>
#include <queue>
#include <set>

using namespace std;

using StructId = string;
using FuncId = string;
using VarId = string;
using FieldId = string;
using BbId = string;

// <{top, bot, int}, "" or "<number>">
using mapValue = pair<string, int>;

inline map<string, mapValue> join(map<string, mapValue>& map1, map<string, mapValue>& map2) {
    map<string, mapValue> newMap;
    for (auto& pair1: map1) {
        mapValue val2 = map2[pair1.first];
        if (pair1.second.first == "int" && val2.first == "int" && pair1.second.second == val2.second) newMap[pair1.first] = make_pair("int", val2.second);
        else if (pair1.second.first == "int" && val2.first == "int" && pair1.second.second != val2.second) newMap[pair1.first] = make_pair("top", 0);
        else if (pair1.second.first == "top" || val2.first == "top") newMap[pair1.first] = make_pair("top", 0);
        else if (pair1.second.first == "bot") newMap[pair1.first] = make_pair(val2.first, val2.second);
        else newMap[pair1.first] = make_pair(pair1.second.first, pair1.second.second);
    }
    return newMap;
}

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
    virtual ~ArithmeticOp() {}
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
    virtual ~ComparisonOp() {}
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
    virtual ~Operand() {}
};
struct Const : Operand{
    int32_t num;
    void print(std::ostream& os) const override { os << num; }
    ~Const() {}
};
struct Var : Operand{
    VarId id;
    void print(std::ostream& os) const override { os << id; }
    ~Var() {}
};

struct Terminal{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
        t.print(os);
        return os;
    }
    virtual ~Terminal() {}
};
struct Branch : Terminal{
    Operand* guard;
    BbId tt;
    BbId ff;
    void print(std::ostream& os) const override {
        os << "Branch(" << *guard << ", " << tt << ", " << ff << ")" << endl;
    }
    ~Branch() { delete guard; }
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
    ~CallDirect() { for (Operand* op: args) delete op; }
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
    ~CallIndirect() { for (Operand* op: args) delete op; }
};
struct Jump : Terminal{
    BbId next_bb;
    void print(std::ostream& os) const override {
        os << "Jump(" << next_bb << ")" << endl;
    }
    ~Jump() {}
};
struct Ret : Terminal{
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Ret(" << *op << ")" << endl;
    }
    ~Ret() { delete op; }
};


struct LirInst{
    virtual void print(std::ostream& os) const {}
    friend std::ostream& operator<<(std::ostream& os, const LirInst& li) {
        li.print(os);
        return os;
    }
    virtual LirInst* transform(map<string, mapValue>& store) { return nullptr; }
    virtual ~LirInst() {}
};
struct Copy : LirInst{
    VarId lhs;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Copy(" << lhs << ", " << *op << ")" << endl;
    }
    ~Copy() { delete op; }
    LirInst* transform(map<string, mapValue>& store) override {
        if (Const* con = dynamic_cast<Const*>(op)) {
            store[lhs] = make_pair("int", con->num);
        }
        else if (Var* var = dynamic_cast<Var*>(op)) {
            store[lhs] = make_pair(store[var->id].first, store[var->id].second);
            if (store[lhs].first == "int") {
                delete op;
                Const* con = new Const;
                con->num = store[lhs].second;
                op = con;
            }
        }
        return nullptr;
    }
};
struct Alloc : LirInst{
    VarId lhs;
    Operand* num;
    void print(std::ostream& os) const override {
        os << "Alloc(" << lhs << ", " << *num << ")" << endl;
    }
    ~Alloc() { delete num; }
};
struct Arith : LirInst{
    VarId lhs;
    ArithmeticOp* aop;
    Operand* left;
    Operand* right;
    void print(std::ostream& os) const override {
        os << "Arith(" << lhs << ", " << *aop << ", " << *left << ", " << *right << ")" << endl;
    }
    ~Arith() { delete aop; delete left; delete right; }
    LirInst* transform(map<string, mapValue>& store) override {
        pair<string, int> leftIdentity;
        if (Const* con = dynamic_cast<Const*>(left)) leftIdentity = make_pair("int", con->num);
        else if (Var* var = dynamic_cast<Var*>(left)) {
            leftIdentity = make_pair(store[var->id].first, store[var->id].second);
            if (leftIdentity.first == "int") {
                Const* con = new Const;
                con->num = leftIdentity.second;
                delete left;
                left = con;
            }
        }
        pair<string, int> rightIdentity;
        if (Const* con = dynamic_cast<Const*>(right)) rightIdentity = make_pair("int", con->num);
        else if (Var* var = dynamic_cast<Var*>(right)) {
            rightIdentity = make_pair(store[var->id].first, store[var->id].second);   
            if (rightIdentity.first == "int") {
                Const* con = new Const;
                con->num = rightIdentity.second;
                delete right;
                right = con;
            }
        }
        
        if (dynamic_cast<Add*>(aop)) {
            if (leftIdentity.first == "int" && leftIdentity.second == 0) {
                Copy* copy = new Copy;
                copy->lhs = lhs;
                copy->op = right;
                right = nullptr;
                store[lhs] = rightIdentity;
                return copy;
            }
            if (rightIdentity.first == "int" && rightIdentity.second == 0) {
                Copy* copy = new Copy;
                copy->lhs = lhs;
                copy->op = left;
                left = nullptr;
                store[lhs] = leftIdentity;
                return copy;
            }
            if (leftIdentity.first == "bot" || rightIdentity.first == "bot") {
                store[lhs] = make_pair("bot", 0);
                return nullptr;
            }
            if (leftIdentity.first == "int" && rightIdentity.first == "int") {
                Copy* copy = new Copy;
                Const* con = new Const;
                con->num = leftIdentity.second + rightIdentity.second;
                copy->lhs = lhs;
                copy->op = con;
                store[lhs] = make_pair("int", con->num);
                return copy;
            }
            if (leftIdentity.first == "top" || rightIdentity.first == "top") {
                store[lhs] = make_pair("top", 0);
                return nullptr;
            }
        }
        if (dynamic_cast<Sub*>(aop)) {
            if (rightIdentity.first == "int" && rightIdentity.second == 0) {
                Copy* copy = new Copy;
                copy->lhs = lhs;
                copy->op = left;
                left = nullptr;
                store[lhs] = leftIdentity;
                return copy;
            }
            if (leftIdentity.first == "bot" || rightIdentity.first == "bot") {
                store[lhs] = make_pair("bot", 0);
                return nullptr;
            }
            if (leftIdentity.first == "int" && rightIdentity.first == "int") {
                Copy* copy = new Copy;
                Const* con = new Const;
                con->num = leftIdentity.second - rightIdentity.second;
                copy->lhs = lhs;
                copy->op = con;
                store[lhs] = make_pair("int", con->num);
                return copy;
            }
            if (leftIdentity.first == "top" || rightIdentity.first == "top") {
                store[lhs] = make_pair("top", 0);
                return nullptr;
            }
        }
        if (dynamic_cast<Mul*>(aop)) {
            if (leftIdentity.first == "int" && leftIdentity.second == 1) {
                Copy* copy = new Copy;
                copy->lhs = lhs;
                copy->op = right;
                right = nullptr;
                store[lhs] = rightIdentity;
                return copy;
            }
            if (rightIdentity.first == "int" && rightIdentity.second == 1) {
                Copy* copy = new Copy;
                copy->lhs = lhs;
                copy->op = left;
                left = nullptr;
                store[lhs] = leftIdentity;
                return copy;
            }
            if (leftIdentity.first == "bot" || rightIdentity.first == "bot") {
                store[lhs] = make_pair("bot", 0);
                return nullptr;
            }
            if (leftIdentity.first == "int" && rightIdentity.first == "int") {
                Copy* copy = new Copy;
                Const* con = new Const;
                con->num = leftIdentity.second * rightIdentity.second;
                copy->lhs = lhs;
                copy->op = con;
                store[lhs] = make_pair("int", con->num);
                return copy;
            }
            if ((leftIdentity.first == "int" && leftIdentity.second == 0) || (rightIdentity.first == "int" && rightIdentity.second == 0)) {
                Copy* copy = new Copy;
                Const* con = new Const;
                con->num = 0;
                copy->lhs = lhs;
                copy->op = con;
                store[lhs] = make_pair("int", 0);
                return copy;
            }
            if (leftIdentity.first == "top" || rightIdentity.first == "top") {
                store[lhs] = make_pair("top", 0);
                return nullptr;
            }
        }
        if (dynamic_cast<Div*>(aop)) {
            if (rightIdentity.first == "int" && rightIdentity.second == 1) {
                Copy* copy = new Copy;
                copy->lhs = lhs;
                copy->op = left;
                left = nullptr;
                store[lhs] = leftIdentity;
                return copy;
            }
            if (leftIdentity.first == "bot" || rightIdentity.first == "bot") {
                store[lhs] = make_pair("bot", 0);
                return nullptr;
            }
            if (rightIdentity.first == "int" && rightIdentity.second == 0) {
                store[lhs] = make_pair("bot", 0);
                return nullptr;
            }
            if (leftIdentity.first == "int" && leftIdentity.second == 0) {
                Copy* copy = new Copy;
                Const* con = new Const;
                con->num = 0;
                copy->lhs = lhs;
                copy->op = con;
                store[lhs] = make_pair("int", 0);
                return copy;
            }
            if (leftIdentity.first == "int" && rightIdentity.first == "int") {
                Copy* copy = new Copy;
                Const* con = new Const;
                con->num = leftIdentity.second / rightIdentity.second;
                copy->lhs = lhs;
                copy->op = con;
                store[lhs] = make_pair("int", con->num);
                return copy;
            }
            if (leftIdentity.first == "top" || rightIdentity.first == "top") {
                store[lhs] = make_pair("top", 0);
                return nullptr;
            }
        }
        return nullptr;
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
    ~CallExt() { for (Operand* op: args) delete op; }
};
struct Cmp : LirInst{
    VarId lhs;
    ComparisonOp* cop;
    Operand* left;
    Operand* right;
    void print(std::ostream& os) const override {
        os << "Cmp(" << lhs << ", " << *cop << ", " << *left << ", " << *right << ")" << endl;
    }
    ~Cmp() { delete cop; delete left; delete right; }
    LirInst* transform(map<string, mapValue>& store) override {
        pair<string, int> leftIdentity;
        if (Const* con = dynamic_cast<Const*>(left)) leftIdentity = make_pair("int", con->num);
        else if (Var* var = dynamic_cast<Var*>(left)) {
            leftIdentity = make_pair(store[var->id].first, store[var->id].second);
            if (leftIdentity.first == "int") {
                Const* con = new Const;
                con->num = leftIdentity.second;
                delete left;
                left = con;
            }
        }
        pair<string, int> rightIdentity;
        if (Const* con = dynamic_cast<Const*>(right)) rightIdentity = make_pair("int", con->num);
        else if (Var* var = dynamic_cast<Var*>(right)) {
            rightIdentity = make_pair(store[var->id].first, store[var->id].second);   
            if (rightIdentity.first == "int") {
                Const* con = new Const;
                con->num = rightIdentity.second;
                delete right;
                right = con;
            }
        }
        if (leftIdentity.first == "bot" || rightIdentity.first == "bot") {
            store[lhs] = make_pair("bot", 0);
            return nullptr;
        }
        if (leftIdentity.first == "top" || rightIdentity.first == "top") {
            store[lhs] = make_pair("top", 0);
            return nullptr;
        }
        Copy* copy = new Copy;
        Const* con = new Const;
        if (dynamic_cast<Equal*>(cop)) con->num = (leftIdentity.second == rightIdentity.second) ? 1 : 0;
        if (dynamic_cast<NotEq*>(cop)) con->num = (leftIdentity.second != rightIdentity.second) ? 1 : 0;
        if (dynamic_cast<Lt*>(cop)) con->num = (leftIdentity.second < rightIdentity.second) ? 1 : 0;
        if (dynamic_cast<Lte*>(cop)) con->num = (leftIdentity.second <= rightIdentity.second) ? 1 : 0;
        if (dynamic_cast<Gt*>(cop)) con->num = (leftIdentity.second > rightIdentity.second) ? 1 : 0;
        if (dynamic_cast<Gte*>(cop)) con->num = (leftIdentity.second >= rightIdentity.second) ? 1 : 0;
        copy->lhs = lhs;
        copy->op = con;
        store[lhs] = make_pair("int", con->num);
        return copy;
    }
};
struct Gep : LirInst{
    VarId lhs;
    VarId src;
    Operand* idx;
    void print(std::ostream& os) const override {
        os << "Gep(" << lhs << ", " << src << ", " << *idx << ")" << endl;
    }
    ~Gep() { delete idx; }
};
struct Gfp : LirInst{
    VarId lhs;
    VarId src;
    string field;
    void print(std::ostream& os) const override {
        os << "Gfp(" << lhs << ", " << src << ", " << field << ")" << endl;
    }
    ~Gfp() {}
};
struct Load : LirInst{
    VarId lhs;
    VarId src;
    void print(std::ostream& os) const override {
        os << "Load(" << lhs << ", " << src << ")" << endl;
    }
    ~Load() {}
};
struct Store : LirInst{
    VarId dst;
    Operand* op;
    void print(std::ostream& os) const override {
        os << "Store(" << dst << ", " << *op << ")" << endl;
    }
    ~Store() { delete op; }
};

struct BasicBlock{
    BbId label;
    vector<LirInst*> insts;
    Terminal* term;
    map<string, mapValue> IN;
    map<string, mapValue> OUT;
    ~BasicBlock() {
        delete term;
        for (LirInst* inst: insts) delete inst;
    }

    void init(vector<pair<VarId, Type*>>& params, map<VarId, Type*>& locals) {
        for (auto& pair: params) {
            if (label == "entry") IN[pair.first] = make_pair("top", 0);
            else IN[pair.first] = make_pair("bot", 0);
            OUT[pair.first] = make_pair("bot", 0);
        }
        for (auto& pair: locals) {
            if (label == "entry") IN[pair.first] = make_pair("int", 0);
            else IN[pair.first] = make_pair("bot", 0);
            OUT[pair.first] = make_pair("bot", 0);
        }
    }

    bool iterate(map<string, mapValue>& store) {
        for (unsigned int i = 0; i < insts.size(); i++) {
            LirInst* newInst = insts[i]->transform(store);
            if (newInst != nullptr) {
                delete insts[i];
                insts[i] = newInst;
            }
        }
        if (Ret* ret = dynamic_cast<Ret*>(term)) {
            if (Var* var = dynamic_cast<Var*>(ret->op)) {
                if (store[var->id].first == "int") {
                    Const* con = new Const;
                    con->num = store[var->id].second;
                    delete ret->op;
                    ret->op = con;
                }
            }
        }
        else if (Branch* branch = dynamic_cast<Branch*>(term)) {
            if (Const* con = dynamic_cast<Const*>(branch->guard)) {
                Jump* jump = new Jump;
                if (con->num == 0) {
                    jump->next_bb = branch->ff;
                }
                else {
                    jump->next_bb = branch->tt;
                }
                delete term;
                term = jump;
            }
            else if (Var* var = dynamic_cast<Var*>(branch->guard)) {
                if (store[var->id].first == "int" && store[var->id].second == 0) {
                    Jump* jump = new Jump;
                    jump->next_bb = branch->ff;
                    delete term;
                    term = jump;
                }
                else if (store[var->id].first == "int") {
                    Jump* jump = new Jump;
                    jump->next_bb = branch->tt;
                    delete term;
                    term = jump;
                }
            }
        }
        bool changed = false;
        for (auto& pair: store) {
            if (OUT[pair.first] != pair.second) changed = true;
        }
        if (changed) OUT = store;        
        return changed;
    }

    friend std::ostream& operator<<(std::ostream& os, const BasicBlock& bb) {
        os << bb.label  << ":" << endl;
        for(unsigned int i = 0; i < bb.insts.size(); i++){
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
    map<BbId, vector<BasicBlock*>> pre;
    set<BbId> used;
    queue<BasicBlock*> q;

    void init() {
        for (auto& pair: body) {
            if (pair.first == "entry") q.push(pair.second);
            if (Jump* jump = dynamic_cast<Jump*>(pair.second->term)) {
                pre[jump->next_bb].push_back(body[pair.first]);
            }
            else if (Branch* branch = dynamic_cast<Branch*>(pair.second->term)) {
                pre[branch->tt].push_back(body[pair.first]);
                pre[branch->ff].push_back(body[pair.first]);
            }
            pair.second->init(params, locals);
        }
    }

    void optimize() {
        while(!q.empty()) {
            BasicBlock* bb = q.front();
            q.pop();
            map<string, mapValue> IN_bb = bb->IN;
            for (BasicBlock* blocks: pre[bb->label]) {
                IN_bb = join(IN_bb, blocks->OUT);
            }
            map<string, mapValue> store = join(IN_bb, IN_bb);
            bool changed = bb->iterate(store);
            if (changed) {
                if (Jump* jump = dynamic_cast<Jump*>(bb->term)) {
                    q.push(body[jump->next_bb]);
                }
                else if (Branch* branch = dynamic_cast<Branch*>(bb->term)) {
                    if (Const* con = dynamic_cast<Const*>(branch->guard)) {
                        if (con->num == 0) q.push(body[branch->ff]);
                        else q.push(body[branch->tt]);
                    }
                    else if (Var* var = dynamic_cast<Var*>(branch->guard)) {
                        if (store[var->id].first == "int" && store[var->id].second == 0) q.push(body[branch->ff]);
                        else if (store[var->id].first == "int") q.push(body[branch->tt]);
                        else if (store[var->id].first == "top") {
                            q.push(body[branch->ff]);
                            q.push(body[branch->tt]);
                        }
                    }
                }
            }
        }
        queue<BbId> imp;
        imp.push("entry");
        while (!imp.empty()) {
            BbId label = imp.front();
            imp.pop();
            if (used.find(label) != used.end()) continue;
            used.insert(label);
            if (Jump* jump = dynamic_cast<Jump*>(body[label]->term)) {
                imp.push(jump->next_bb);
            }
            else if (Branch* branch = dynamic_cast<Branch*>(body[label]->term)) {
                imp.push(branch->tt);
                imp.push(branch->ff);
            }
        }

        auto it = body.begin();
        while (it != body.end()) {
            if (used.find(it->first) == used.end()) {
                delete body[it->first];
                body.erase(it->first);
                it = body.begin();
            }
            else it++;
        }
    }

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
};

struct Program{
    map<VarId, Type*> globals;    
    map<StructId, map<FieldId, Type*>>structs;
    map<FuncId, Type*> externs;
    map<FuncId, Function*> functions;

    ~Program() { 
    }

    void optimize() {
        for(auto& f: functions){
            if (f.first == "test") {
                f.second->init();
                f.second->optimize();
            }
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Program& prog) {

        for(auto f: prog.functions){
            if (f.first == "test") os << *f.second << endl;
        }
        
        return os;
    }
};