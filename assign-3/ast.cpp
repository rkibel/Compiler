#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>
using namespace std;

#ifndef AST_CPP
#define AST_CPP

struct TypeName {
    string type_name;
    string struct_name; // used to pass in for exp or *
    string field_name;  // used to pass in for exp or *
    TypeName( string t, string s = "", string f = "") : type_name(t), struct_name(s), field_name(f) {};
    TypeName(const TypeName& other) : type_name(other.type_name) {}
    TypeName& operator=(const TypeName& other) {
        if (this != &other) type_name = other.type_name;
        return *this;
    }
    bool operator==(const TypeName& other) const {
        try {
            if (type_name == "_" || other.type_name == "_") return true;
            if (type_name == other.type_name) return true;
            return type_name.at(0) == '&' && other.type_name.at(0) == '&' && (type_name.at(1) == '_' || other.type_name.at(1) == '_');
        } catch (const out_of_range& e) {} 
        return false;
    }
    bool operator!=(const TypeName& other) const {
        return !(*this == other);
    }
    bool isFunction() const { return type_name[0] == '(' || type_name.substr(0,2) == "&("; }
    bool isNonPointerFunction() const { return type_name[0] == '('; }
    bool isStruct() const { return type_name[0] != '&' && type_name.find('(') == string::npos && type_name.substr(0,3) != "int"; }
    bool isValidFieldAcesss() const { return type_name[0] == '&' && type_name[1] != '(' && type_name[1] != '&' && type_name.substr(0,5) != "&int"; }
    bool isPointerToFunction() const { return type_name[0] == '(' || type_name.find("&(") != string::npos; }
};

namespace AST {

struct Exp;
struct Type;
struct Function;
struct Lval;

using ParamsReturnVal = pair<vector<Type*>, Type*>;
using Gamma = unordered_map<string, Type*>;
using Delta = unordered_map<string, Gamma>;
using Errors = vector<string>;
using FunctionsInfo = unordered_map<string, ParamsReturnVal>;
using StructFunctionsInfo = unordered_map<string, FunctionsInfo>;

struct Type {
    virtual void print(ostream& os) const {}
    virtual bool isAny() const { return false; }
    virtual TypeName typeName() const { return TypeName("_"); }
    virtual ParamsReturnVal funcInfo() const;
    virtual ~Type() {}
    friend ostream& operator<<(ostream& os, const Type& obj) {
        obj.print(os);
        return os;
    }
    virtual string toLIRType() const { return ""; }
};
struct Int : Type {
    void print(ostream& os) const override {
        os << "Int";
    }
    TypeName typeName() const override {
        return TypeName("int");
    }
    string toLIRType() const override { return "Int"; }
};
struct StructType : Type {
    string name;
    void print(ostream& os) const override { 
        os << "Struct(" << name << ")"; 
    }
    TypeName typeName() const override {
        return TypeName(name);
    }
    string toLIRType() const override { return "Struct(" + name + ")"; }
};
struct Fn : Type {
    vector<Type*> prms;
    // WARNING: if there is no return for Fn, ret is Any (look at struct Any : Type)
    Type* ret;
    void print(ostream& os) const override { 
        os << "Fn(prms = [";
        for (unsigned int i = 0; i < prms.size(); i++) {
            os << *prms[i];
            if (i != prms.size() - 1) os << ", ";
        }
        os << "], ret = " << *ret << ")";
    }
    TypeName typeName() const override {
        string prms_type_names;
        for (unsigned int i = 0; i < prms.size(); i++) {
            prms_type_names += prms[i]->typeName().type_name;
            if (i != prms.size() - 1) prms_type_names += ", ";
        }
        string temp = "(" + prms_type_names + ") -> " + ret->typeName().type_name;
        return TypeName(temp);
    }
    ParamsReturnVal funcInfo () const override { return make_pair(prms, ret); }
    ~Fn() {
        for (Type* t: prms) delete t;
        delete ret;
    }
    string toLIRType() const override { 
        string res = "Fn([";
        for (unsigned int i = 0; i < prms.size(); i++) {
            res += prms[i]->toLIRType();
            if (i != prms.size() - 1) res += ", ";
        }
        res += "], " + ret->toLIRType() + ")";
        return res;
    }
};
struct Ptr : Type {
    Type* ref;
    void print(ostream& os) const override { 
        os << "Ptr(" << *ref << ")";
    }
    TypeName typeName() const override {
        string ptr_type_names;
        const Type* temp = ref;
        while (temp) {
            ptr_type_names += "&";
            if (dynamic_cast<const Ptr*>(temp)) { // Checks if temp is a pointer type
                temp = static_cast<const Ptr*>(temp)->ref;
            } else {
                ptr_type_names += temp->typeName().type_name;
                break;
            }
        }
        return TypeName(ptr_type_names);
    }
    ParamsReturnVal funcInfo () const override { return ref->funcInfo(); }
    ~Ptr() { delete ref; }
    string toLIRType() const override {  return "Ptr(" + ref->toLIRType() + ")"; }
};
struct Any : Type {
    void print(ostream& os) const override { 
        os << "_";
    }
    TypeName typeName() const override {
        return TypeName("_");
    }
    bool isAny() const override { return true; }
    string toLIRType() const override { return "_"; }
};

struct UnaryOp {
    virtual void print(ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const { return TypeName("_"); };
    friend ostream& operator<<(ostream& os, const UnaryOp& uo) {
        uo.print(os);
        return os;
    }
    virtual string toLIRType() const { return ""; }
};
struct Neg : UnaryOp {
    void print(ostream& os) const override { os << "Neg"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const override; //Defined at the bottom
    string toLIRType() const override { return "neg"; }
};
struct UnaryDeref : UnaryOp {
    void print(ostream& os) const override { os << "Deref"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const override;
    string toLIRType() const override { return "deref"; }
};

struct BinaryOp{
    virtual void print(ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const; //define for rest, overload for eq/not eq (defined below)
    friend ostream& operator<<(ostream& os, const BinaryOp& bo) {
        bo.print(os);
        return os;
    }
    virtual string toLIRType() const { return ""; }
};
struct Add : BinaryOp{
    void print(ostream& os) const override { os << "Add"; }
    string toLIRType() const override { return "add"; }
};
struct Sub : BinaryOp{
    void print(ostream& os) const override { os << "Sub"; }
    string toLIRType() const override { return "sub"; }
};
struct Mul : BinaryOp{
    void print(ostream& os) const override { os << "Mul"; }
    string toLIRType() const override { return "mul"; }
};
struct Div : BinaryOp{
    void print(ostream& os) const override { os << "Div"; }
    string toLIRType() const override { return "div"; }
};
struct Equal : BinaryOp{
    void print(ostream& os) const override { os << "Equal"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const override;
    string toLIRType() const override { return "eq"; }
};
struct NotEq : Equal{ //so the same typeCheck function is inherited
    void print(ostream& os) const override { os << "NotEq"; }
    string toLIRType() const override { return "neq"; }
};
struct Lt : BinaryOp{
    void print(ostream& os) const override { os << "Lt"; }
    string toLIRType() const override { return "lt"; }
};
struct Lte : BinaryOp{
    void print(ostream& os) const override { os << "Lte"; }
    string toLIRType() const override { return "lte"; }
};
struct Gt : BinaryOp{
    void print(ostream& os) const override { os << "Gt"; }
    string toLIRType() const override { return "gt"; }
};
struct Gte : BinaryOp{
    void print(ostream& os) const override { os << "Gte"; }
    string toLIRType() const override { return "gte"; }
};

struct Exp {
    virtual void print(ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual string getName() { return "_"; };
    virtual bool isAny() const { return false; }
    virtual bool isFieldAccess() const { return false; }
    virtual bool isId() const { return false; }
    virtual Exp* getPtr() { return nullptr; }
    virtual ~Exp() {}
    friend ostream& operator<<(ostream& os, const Exp& exp) {
        exp.print(os);
        return os;
    }
    virtual tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const { return make_tuple(0, "", ""); }
};
struct Num : Exp {
    int32_t n;
    void print(ostream& os) const override { os << "Num(" << n << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override { return TypeName("int"); };
    string getName() override { return "int"; }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        return make_tuple(0, to_string(n), "Num");
    }
};
struct ExpId : Exp {
    string name;
    ExpId() {}
    ExpId(const string& name) : name(name) {}
    void print(ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    string getName() override { return name; }
    virtual bool isId() const { return true; }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        return make_tuple(0, name, gamma[name]->toLIRType()); 
    }
};
struct Nil : Exp {
    void print(ostream& os) const override { os << "Nil"; }
    TypeName typeName() const { return TypeName("&_"); }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override { return TypeName("&_");};
    string getName() override { return "_"; }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        return make_tuple(0, "0", "_"); 
    }
};
struct UnOp : Exp {
    UnaryOp* op;
    Exp* operand;
    void print(ostream& os) const override { os << *op << "(" << *operand << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, operand);
    }
    string getName() override { return operand->getName(); }
    ~UnOp() { delete operand; }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        string res = "";
        string lhs_type = "";
        if (op->toLIRType() == "neg") {
            tempsToType.push_back("!");
            unsigned int fresh_var = tempsToType.size();
            auto [eval_var_src, eval_string_src, op_type] = operand->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
            if (eval_var_src != 0) {
                res += eval_string_src; //building the res instructions
                eval_string_src = "_t" + to_string(eval_var_src); //now used for old var
            }
            lhs_type = "Int";
            tempsToType[fresh_var-1] = lhs_type;
            res += "    Arith(_t" + to_string(fresh_var) + ", sub, 0, " + eval_string_src + ")\n";                  
            return make_tuple(fresh_var, res, lhs_type);
        }
        else {
            auto [eval_var_src, eval_string_src, op_type] = operand->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
            if (eval_var_src != 0) {
                res += eval_string_src; //building the res instructions
                eval_string_src = "_t" + to_string(eval_var_src);
            }
            lhs_type = op_type.substr(4, op_type.length() - 5);
            tempsToType.push_back(lhs_type);
            unsigned int fresh_var = tempsToType.size();
            res += "    Load(_t" + to_string(fresh_var) + ", " + eval_string_src + ")\n";  
            return make_tuple(fresh_var, res, lhs_type); //returning new type
        }
        return make_tuple(0, "", ""); 
    }
};
struct BinOp : Exp {
    BinaryOp* op;
    Exp* left;
    Exp* right;
    void print(ostream& os) const override {
        os << "BinOp(\nop = " << *op << ",\nleft = " << *left << ",\nright = " << *right << "\n)"; 
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, left, right);
    }
    string getName() override { return "_"; }
    ~BinOp() { delete left; delete right; }
    
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        auto [eval_var_left, eval_string_left, _] = left->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        auto [eval_var_right, eval_string_right, _2] = right->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        string opType = op->toLIRType();
        string res = "";
        string lhs_type = "Int";
        if (eval_var_left != 0) {
            res += eval_string_left;
            eval_string_left = "_t" + to_string(eval_var_left);
        }
        if (eval_var_right != 0) {
            res += eval_string_right;
            eval_string_right = "_t" + to_string(eval_var_right);
        }
        tempsToType.push_back(lhs_type);
        unsigned int fresh_var = tempsToType.size();
        if (opType == "add" || opType == "sub" || opType == "mul" || opType == "div") {
            res += "    Arith(_t" + to_string(fresh_var) + ", " + opType + ", " + eval_string_left + ", " + eval_string_right + ")\n";
        }
        else {
            res += "    Cmp(_t" + to_string(fresh_var) + ", " + opType + ", " + eval_string_left + ", " + eval_string_right + ")\n";
        }
        return make_tuple(fresh_var, res, lhs_type);        
    }
};
struct ExpArrayAccess : Exp {
    Exp* ptr;
    Exp* index;
    void print(ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    string getName() override { return ptr->getName(); }
    ~ExpArrayAccess() { delete ptr; delete index; }
    
    // TODO this method
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        auto [eval_var_src, eval_string_src, src_type] = ptr->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        auto [eval_var_idx, eval_string_idx, _] = index->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        string res = "";
        string lhs_type = "";
        if (eval_var_src != 0) {
            res += eval_string_src;
            eval_string_src = "_t" + to_string(eval_var_src);
        }
        if (eval_var_idx != 0) {
            res += eval_string_idx;
            eval_string_idx = "_t" + to_string(eval_var_idx);
        }
        tempsToType.push_back(src_type);
        unsigned int fresh_var_elem = tempsToType.size();
        lhs_type = src_type.substr(1);
        tempsToType.push_back(lhs_type);
        unsigned int fresh_var_lhs = tempsToType.size();
        res += "    Gep(_t" + to_string(fresh_var_elem) + ", " + eval_string_src + ", " + eval_string_idx + ")\n";
        res += "    Load(_t" + to_string(fresh_var_lhs) + ", _t" + to_string(fresh_var_elem) + ")\n";
        return make_tuple(fresh_var_lhs, res, lhs_type);
    }
};
struct ExpFieldAccess : Exp {
    Exp* ptr;
    string field;
    void print(ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    bool isFieldAccess() const override { return true; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    string getName() override { return field; }
    Exp* getPtr() override { return ptr; }
    ~ExpFieldAccess() { delete ptr; }
    
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const;
};
struct ExpCall : Exp {
    Exp* callee;
    vector<Exp*> args;
    void print(ostream& os) const override {
        os << "Call(\ncallee = " << *callee << ",\nargs = [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *args[i];
            if (i != args.size() - 1) os << ", ";
        }
        os << "\n]\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~ExpCall() {
        delete callee;
        for (Exp* exp: args) delete exp;
    }
    string getName() override { return callee->getName(); }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override;
};
struct AnyExp : Exp {
    void print(ostream& os) const override { os << "_"; }
    TypeName typeName() const { return TypeName("_"); }
    bool isAny() const override { return true; }
    string getName() override { return "_"; }
    // TODO this method
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        return make_tuple(0, "_", "_"); 
    }
};
struct Lval {
    virtual void print(ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual string getName() { return "_"; }
    virtual bool isFieldAccess() const { return false; }
    virtual ~Lval() {}
    friend ostream& operator<<(ostream& os, const Lval& lval) {
        lval.print(os);
        return os;
    }
    virtual tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const {
        return make_tuple(0, "_", "_"); 
    }
};
struct LvalId : Lval {
    string name;
    void print(ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    string getName() override { return name; }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override {
        return make_tuple(0, name, gamma[name]->toLIRType()); 
    }
};
struct LvalDeref : Lval {
    Lval* lval;
    void print(ostream& os) const override { os << "Deref(" << *lval << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    string getName() override { return lval->getName(); }
    ~LvalDeref() { delete lval; }
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override;
};
struct LvalArrayAccess : Lval {
    Lval* ptr;
    Exp* index;
    void print(ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    string getName() override { return ptr->getName(); }
    ~LvalArrayAccess() { delete ptr; delete index; }
    // TODO this method
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override;
};
struct LvalFieldAccess : Lval {
    Lval* ptr;
    string field;
    void print(ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    string getName() override { return ptr->getName(); }
    bool isFieldAccess() const override { return false;  }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~LvalFieldAccess() { delete ptr; }
    // TODO this method
    tuple<unsigned int, string, string> lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const override;
};


struct Rhs {
    virtual void print(ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual ~Rhs() {}
    friend ostream& operator<<(ostream& os, const Rhs& rhs) {
        rhs.print(os);
        return os;
    }
};
struct RhsExp : Rhs {
    Exp* exp;
    void print(ostream& os) const override { os << *exp; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~RhsExp() { delete exp; }
};
struct New : Rhs {
    Type* type;
    Exp* amount;
    void print(ostream& os) const override { os << "New(" << *type << ", " << *amount << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~New() { delete type; delete amount; }
};

struct Stmt {
    virtual void print(ostream& os) const {}
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const { return true; };
    virtual string getName() { return "_"; }
    virtual ~Stmt() {}
    friend ostream& operator<<(ostream& os, const Stmt& stmt) {
        stmt.print(os);
        return os;
    }
    virtual string lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, unsigned int& prevWhileHdr, unsigned int& prevWhileEnd, map<string, string>& extern_map, map<string, string>& function_map) const {
        return "?\n"; 
    }
};
struct Break : Stmt {
    void print(ostream& os) const override { os << "Break"; }
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
};
struct Continue : Stmt {
    void print(ostream& os) const override { os << "Continue"; }
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
};
struct Return : Stmt {
    // WARNING: if there is no return, exp is AnyExp : Exp
    Exp* exp;
    void print(ostream& os) const override {
        os << "Return(" << *exp << ")";
    }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~Return() { delete exp; }
    string lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, unsigned int& prevWhileHdr, unsigned int& prevWhileEnd, map<string, string>& extern_map, map<string, string>& function_map) const override {
        if (typeid(*exp) == typeid(AnyExp)) {
            return "    Ret(_)\n";
        }
        auto [temp_var, res_string, _] = exp->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        string res = "";
        if (temp_var != 0) {res += res_string; res_string = "_t" + to_string(temp_var); }
        res += "    Ret(" + res_string + ")\n";
        return res;
    }
};
struct Assign : Stmt {
    Lval* lhs;
    Rhs* rhs;
    void print(ostream& os) const override {
        os << "Assign(\nlhs = " << *lhs << ",\nrhs = " << *rhs << "\n)";
    }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~Assign() { delete lhs; delete rhs; }
};
struct StmtCall : Stmt {
    Lval* callee;
    vector<Exp*> args;
    void print(ostream& os) const override {
        os << "Call(\ncallee = " << *callee << ",\nargs = [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "]\n)";
    }
    string getName() override { return callee->getName(); }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~StmtCall() { delete callee; for (Exp* exp: args) delete exp; }
};
struct If : Stmt {
    Exp* guard;
    vector<Stmt*> tt;
    vector<Stmt*> ff;
    void print(ostream& os) const override {
        os << "If(\nguard = " << *guard << ",\ntt = [";
        for (unsigned int i = 0; i < tt.size(); i++) {
            os << *(tt[i]);
            if (i != tt.size() - 1) os << ", ";
        }
        os << "],\nff = [";
        for (unsigned int i = 0; i < ff.size(); i++) {
            os << *(ff[i]);
            if (i != ff.size() - 1) os << ", ";
        }
        os << "]\n)";
    }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~If() { delete guard; for (Stmt* stmt: tt) delete stmt; for (Stmt* stmt: ff) delete stmt; }
};
struct While : Stmt {
    Exp* guard;
    vector<Stmt*> body;
    void print(ostream& os) const override {
        os << "While(\nguard = " << *guard << ",\nbody = [";
        for (unsigned int i = 0; i < body.size(); i++) {
            os << *(body[i]);
            if (i != body.size() - 1) os << ", ";
        }
        os << "]\n)";
    }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~While() { delete guard; for (Stmt* stmt: body) delete stmt; }
};

struct Decl {
    // WARNING: should only call its own typename
    string name;
    Type* type; // Fn type if this Decl is an extern
    vector<Decl*> params; // optional, used to keep track of parameters for extern decl
    ~Decl() { delete type; }
    friend ostream& operator<<(ostream& os, const Decl& decl) {
        os << "Decl(" << decl.name << ", " << *(decl.type) << ")";
        return os;
    }
    TypeName typeName() const { return type->typeName(); }
    ParamsReturnVal funcInfo () const { return type->funcInfo(); }
};

struct Struct {
    string name;
    vector<Decl*> fields;
    ~Struct() { for (Decl* decl: fields) delete decl; }
    friend ostream& operator<<(ostream& os, const Struct& str) {
        os << "Struct(\nname = " << str.name << ",\nfields = [";
        for (unsigned int i = 0; i < str.fields.size(); i++) {
            os << *(str.fields[i]);
            if (i != str.fields.size() - 1) os << ", ";
        }
        os << "]\n)";
        return os;
    }
    TypeName typeName() const { return TypeName(name); }
    bool typeCheck(Gamma& gamma, Errors& errors) const;
};

struct Function {
    string name;
    vector<Decl*> params;
    // WARNING: if function has no return type, rettyp is Any : Type
    Type* rettyp;
    // WARNING: for each local, if there is no value after declaration, Exp is AnyExp : Exp
    vector<pair<Decl*, Exp*>> locals;
    vector<Stmt*> stmts;
    string toLIRType() const {
        string res = "Fn([";
        for (unsigned int i = 0; i < params.size(); i++) {
            res += params[i]->type->toLIRType();
            if (i != params.size() - 1) res += ", ";
        }
        res += "], " + rettyp->toLIRType() + ")";
        return res;
    }
    ~Function() {
        for (Decl* decl: params) delete decl;
        delete rettyp;
        for (auto [decl, exp]: locals) { delete decl; delete exp; }
        for (Stmt* stmt: stmts) delete stmt;
    }
    friend ostream& operator<<(ostream& os, const Function& func) {
        os << "Function(\nname = " << func.name << ",\nparams = [";
        for (unsigned int i = 0; i < func.params.size(); i++) {
            os << *(func.params[i]);
            if (i != func.params.size() - 1) os << ", ";
        }
        os << "],\nrettyp = " << *(func.rettyp) << ",\nlocals = [";
        for (unsigned int i = 0; i < func.locals.size(); i++) {
            os << "(" << *(func.locals[i].first) << ", " << *(func.locals[i].second) << ")";
            if (i != func.locals.size() - 1) os << ", ";
        }
        os << "],\nstmts = [";
        for (unsigned int i = 0; i < func.stmts.size(); i++) {
            os << *(func.stmts[i]);
            if (i != func.stmts.size() - 1) os << ", ";
        }
        os << "]\n)";
        return os;
    }
    TypeName typeName() const {
        string prms_type_names;
        for (unsigned int i = 0; i < params.size(); i++) {
            prms_type_names += params[i]->typeName().type_name;
            if (i != params.size() - 1) prms_type_names += ", ";
        }
        return TypeName("&(" + prms_type_names + ") -> " + rettyp->typeName().type_name); //Functions should return a pointer to a function
    }
    ParamsReturnVal funcInfo () const { 
        vector<Type*> prms;
        for (Decl* decl: params) {
            prms.push_back(decl->type);
        }
        return make_pair(prms, rettyp); 
    }
    Type* functionType() {
        Fn* fun = new Fn();
        for (Decl* decl: params) {
            fun->prms.push_back(decl->type);
        }
        fun->ret = rettyp;
        return static_cast<Type*>(fun);
    }
    bool typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const;
};

struct Program {
    vector<Decl*> globals;
    vector<Struct*> structs;
    vector<Decl*> externs;
    vector<Function*> functions;
    ~Program() {
        for (Decl* decl: globals) delete decl;
        for (Struct* str: structs) delete str;
        for (Decl* decl: externs) delete decl;
        for (Function* function: functions) delete function;
    }
    friend ostream& operator<<(ostream& os, const Program& prog) {
        os << "Program(\nglobals = [";
        for (unsigned int i = 0; i < prog.globals.size(); i++) {
            os << *(prog.globals[i]);
            if (i != prog.globals.size() - 1) os << ", ";
        }
        os << "],\nstructs = [";
        for (unsigned int i = 0; i < prog.structs.size(); i++) {
            os << *(prog.structs[i]);
            if (i != prog.structs.size() - 1) os << ", ";
        }
        os << "],\nexterns = [";
        for (unsigned int i = 0; i < prog.externs.size(); i++) {
            os << *(prog.externs[i]);
            if (i != prog.externs.size() - 1) os << ", ";
        }
        os << "],\nfunctions = [";
        for (unsigned int i = 0; i < prog.functions.size(); i++) {
            os << *(prog.functions[i]);
            if (i != prog.functions.size() - 1) os << ", ";
        }
        os << "]\n)";
        return os;
    }
    bool typeCheck(Gamma& gamma, Errors& errors, unordered_map<string, Gamma>& locals_map) const;
};

}

using namespace AST;
using ParamsReturnVal = pair<vector<Type*>, Type*>;
using Gamma = unordered_map<string, Type*>;
using Delta = unordered_map<string, Gamma>;
using Errors = vector<string>;
using FunctionsInfo = unordered_map<string, ParamsReturnVal>;
using StructFunctionsInfo = unordered_map<string, FunctionsInfo>;

extern Delta delta;
extern FunctionsInfo functions_map;
extern StructFunctionsInfo struct_functions_map;

// General functions (with _TC for ones used by both Exp and Stmt) which depend on earlier definitions
ParamsReturnVal Type::funcInfo () const {
    return make_pair(vector<Type*>(), new Any());
}

TypeName RhsExp::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return exp->typeCheck(gamma, fun, errors);
}

TypeName const id_TC(Gamma& gamma, const Function* fun, Errors& errors, string name) {
    try {
        return TypeName(gamma.at(name)->typeName()); //Returns TypeName* struct
    } catch (const out_of_range& e) {
        errors.push_back("[ID] in function " + fun->name + ": variable " + name + " undefined");
        return TypeName("_");
    }
}
TypeName ExpId::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return id_TC(gamma, fun, errors, name);
};
TypeName LvalId::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return id_TC(gamma, fun, errors, name);
};

TypeName const deref_TC(Gamma& gamma, const Function* fun, Errors& errors, variant<Exp*, Lval*> operand) {
    TypeName exp_type = visit([&exp_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, operand);
    if (exp_type.type_name == "_") { return exp_type; }
    if ( exp_type != TypeName("&_") && exp_type.type_name[0] != '&') {
        errors.push_back("[DEREF] in function " + fun->name + ": dereferencing type " + exp_type.type_name + " instead of pointer");
        return TypeName("_");
    }
    return TypeName( exp_type.type_name.substr(1) );
}
TypeName UnaryDeref::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const {
    return deref_TC(gamma, fun, errors, operand);
}
TypeName LvalDeref::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return deref_TC(gamma, fun, errors, lval);
}

TypeName Neg::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const {
    TypeName exp_type ( operand->typeCheck(gamma, fun, errors) );
    if (exp_type != TypeName("int")) {
        errors.push_back("[NEG] in function " + fun->name + ": negating type " + exp_type.type_name + " instead of int");
    }
    return TypeName("int");
};

TypeName BinaryOp::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, fun, errors) );
    TypeName right_type ( right->typeCheck(gamma, fun, errors) );
    if ( left_type != TypeName("int") ) {
        errors.push_back("[BINOP-REST] in function " + fun->name + ": operand has type " + left_type.type_name + " instead of int");
    }
    if ( right_type != TypeName("int") ) {
        errors.push_back("[BINOP-REST] in function " + fun->name + ": operand has type " + right_type.type_name + " instead of int");
    }
    return TypeName("int");
}

TypeName Equal::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, fun, errors) );
    TypeName right_type ( right->typeCheck(gamma, fun, errors) );
    if (left_type != TypeName("int") && left_type.type_name[0] != '&') {
        errors.push_back("[BINOP-EQ] in function " + fun->name + ": operand has non-primitive type " + left_type.type_name);
    }
    if (right_type != TypeName("int") && right_type.type_name[0] != '&') {
        errors.push_back("[BINOP-EQ] in function " + fun->name + ": operand has non-primitive type " + right_type.type_name);
    }
    if (left_type != right_type) {
        errors.push_back("[BINOP-EQ] in function " + fun->name + ": operands with different types: " + left_type.type_name + " vs " + right_type.type_name);
    }
    return TypeName("int");
}

TypeName const arrayAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, variant<Exp*, Lval*> ptr, Exp* index) {
    TypeName ptr_type = visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    TypeName index_type = index->typeCheck(gamma, fun, errors);
    if (index_type != TypeName("int")) {
        errors.push_back("[ARRAY] in function " + fun->name + ": array index is type " + index_type.type_name + " instead of int");
    }
    if (ptr_type.type_name == "_") { return TypeName("_"); }
    if (ptr_type.type_name != "_" && ptr_type.type_name[0] != '&') { //don't have to worry about dereferencing nil
        errors.push_back("[ARRAY] in function " + fun->name + ": dereferencing non-pointer type " + ptr_type.type_name);
        return TypeName("_");
    }
    return TypeName( ptr_type.type_name.substr(1) );
}
TypeName ExpArrayAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return arrayAccess_TC(gamma, fun, errors, ptr, index);
}
TypeName LvalArrayAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return arrayAccess_TC(gamma, fun, errors, ptr, index);
}

TypeName const fieldAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, variant<Exp*, Lval*> ptr, string field) {
    TypeName ptr_type = visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    if (ptr_type.type_name == "_") { return TypeName("_"); } //errors won't happpen given Any struct
    
    string struct_type = ptr_type.type_name.substr(1);    
    if ( !ptr_type.isValidFieldAcesss()) { //if accessing something other than a struct type
        errors.push_back("[FIELD] in function " + fun->name + ": accessing field of incorrect type " + ptr_type.type_name);
        return TypeName("_"); //all three errors are mutually exclusive
    }
    if (delta.find(struct_type) == delta.end() ) {  // If the iterator points to the end of the map, the key doesn't exist
        errors.push_back("[FIELD] in function " + fun->name + ": accessing field of non-existent struct type " + struct_type);
        return TypeName("_");
    }
    if (delta[struct_type].find(field) == delta[struct_type].end()) {
        errors.push_back("[FIELD] in function " + fun->name + ": accessing non-existent field " + field + " of struct type " + struct_type);
        return TypeName("_");
    }    
    return TypeName(delta[struct_type][field]->typeName().type_name, struct_type, field);
}
TypeName ExpFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName temp = fieldAccess_TC(gamma, fun, errors, ptr, field);
    return temp;
}
TypeName LvalFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName temp = fieldAccess_TC(gamma, fun, errors, ptr, field);
    return temp;
}

pair<TypeName, bool> call_TC(Gamma& gamma, const Function* fun, Errors& errors, variant<Exp*, Lval*> callee, vector<Exp*> args) {    
    TypeName callee_type = visit([&callee_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, callee);
    bool isFieldAccess = visit([](auto* arg) { return arg->isFieldAccess(); }, callee);
    string callee_name = visit([](auto* arg) { return arg->getName(); }, callee); //field name for field, function for others
    
    if (callee_name == "_") { return pair<TypeName, bool>(TypeName("_"), true); } //no need to continue if Any
    bool success = true;
    
    string expression_statement = holds_alternative<Exp*>(callee) ? "[ECALL" : "[SCALL";
    if (callee_name == "main") { //If so, can't be an extern call
        errors.push_back(expression_statement+"-INTERNAL] in function " + fun->name + ": calling main");
        success = false;
    }
    if (callee_type.type_name == "_") { return pair<TypeName, bool>(TypeName("_"), true); }
    if (!callee_type.isFunction()) { //main can be a parameter, so should check if bad type
        errors.push_back(expression_statement+"-*] in function " + fun->name + ": calling non-function type " + callee_type.type_name);
        return pair<TypeName, bool>(TypeName("_"), false);
    }
    if (callee_name == "main" && functions_map.find("main") == functions_map.end()) { //if main not defined as a function, return
            return pair<TypeName, bool>(TypeName("_"), false);
    }
    if (callee_type.type_name == "_") { return pair<TypeName, bool>(TypeName("_"), true); } //callee is just any, just return undefined error which typeCheck already added (and possibly main error)
    
    ParamsReturnVal prv;
    
    if ( isFieldAccess ) { //can assume struct name exists, not field
        string struct_name = callee_type.struct_name;
        string field_name = callee_type.field_name;
        prv = struct_functions_map[struct_name][field_name];
    } else { //can assume it exists since we already returned for values not in gamma
        prv = functions_map[callee_name]; 
    }
    
    string internal_external = ( callee_type.type_name[0] == '&' ) ? "-INTERNAL]" : "-EXTERN]";
    string error_type = expression_statement + internal_external;
    if (expression_statement != "[SCALL" && prv.second->typeName().type_name == "_") { // empty return type
        errors.push_back(error_type + " in function " + fun->name + ": calling a function with no return value");
        success = false;
    }
    if (prv.first.size() != args.size()) {
        errors.push_back(error_type + " in function " + fun->name + 
        ": call number of arguments (" + to_string(args.size()) + ") and parameters (" + to_string(prv.first.size()) + ") don't match");
        success = false;
    }
    unsigned int min = (prv.first.size() < args.size()) ? prv.first.size() : args.size();
    for (unsigned int i = 0; i < min; i++) {
        TypeName param_type = prv.first[i]->typeName();
        TypeName arg_type = args[i]->typeCheck(gamma, fun, errors);
        if (param_type != arg_type) {
            errors.push_back(error_type + " in function " + fun->name + ": call argument has type " + arg_type.type_name 
                + " but parameter has type " + param_type.type_name);
            success = false;
        }
    }
    pair<TypeName, bool> temp(prv.second->typeName(), success);
    return pair<TypeName, bool>(prv.second->typeName(), success);
}

TypeName ExpCall::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return call_TC(gamma, fun, errors, callee, args).first;
}

bool StmtCall::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    return call_TC(gamma, fun, errors, callee, args).second;
}

bool Return::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    bool is_return_type_any = fun->rettyp->isAny();
    bool is_return_exp_any = exp->isAny();
    TypeName return_type = fun->rettyp->typeName();
    TypeName exp_type = exp->typeCheck(gamma, fun, errors); //store typename given, typeCheck will replace undefined variables with Any
    if ( !is_return_exp_any && exp_type.type_name == "_") { 
        if ( is_return_type_any ) {
            errors.push_back("[RETURN-1] in function " + fun->name + ": should return nothing but returning " + exp_type.type_name);
        } else { return true; }
    } //If exp was made to be Any in typeCheck
    if ( exp_type.type_name != return_type.type_name) { //If not equal, then at least one of the types not _ and they are different
        if ( is_return_type_any && !is_return_exp_any ) { //If it wasnt Any before
            errors.push_back("[RETURN-1] in function " + fun->name + ": should return nothing but returning " + exp_type.type_name);
        } else if ( return_type.type_name != "_" && exp_type.type_name == "_") { //If it was Any before
            errors.push_back("[RETURN-2] in function " + fun->name + ": should return " + return_type.type_name + " but returning nothing");
        } else if (exp_type != return_type) {
            errors.push_back("[RETURN-2] in function " + fun->name + ": should return " + return_type.type_name + " but returning " + exp_type.type_name);
        }
        return false;
    }
    return true;
};

TypeName New::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName amount_type = amount->typeCheck(gamma, fun, errors);
    TypeName type_typename = type->typeName();
    if((amount_type.type_name != "int") && (amount_type.type_name != "_")) {
        errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": allocation amount is type " + amount_type.type_name + " instead of int");
    }
    if(type_typename.isNonPointerFunction()) {
        errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": allocating function type " + type_typename.type_name);
    }
    return TypeName("new " + type_typename.type_name);
}

bool Break::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    if(!loop){
        errors.push_back("[BREAK] in function " + fun->name + ": break outside of loop");
        return false;
    } else{ return true; }
}

bool Continue::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    if(!loop){
        errors.push_back("[CONTINUE] in function " + fun->name + ": continue outside of loop");
        return false;
    } else { return true; }
}

bool Assign::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    TypeName lhs_type = lhs->typeCheck(gamma, fun, errors);
    TypeName rhs_type = rhs->typeCheck(gamma, fun, errors);
    bool tf = true;
    if((rhs_type.type_name == "new _") || lhs_type == rhs_type){ return true;} 
    else {
        if(rhs_type.type_name.substr(0,3) != "new") { //not new section
            if((lhs_type.type_name != rhs_type.type_name)){ //already not any from top if statement
                errors.push_back("[ASSIGN-EXP] in function " + fun->name + ": assignment lhs has type " + lhs_type.type_name + " but rhs has type " + rhs_type.type_name);
                tf = false;
            }
            if(lhs_type.isNonPointerFunction() || lhs_type.isStruct()) {
                errors.push_back("[ASSIGN-EXP] in function " + fun->name + ": assignment to struct or function");
                tf = false;
            }
        } 
        else if (lhs_type.type_name == rhs_type.type_name.substr(4)) {
            errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": assignment lhs has type " + lhs_type.type_name + " but we're allocating type " + rhs_type.type_name.substr(4));
            tf = false;
        }
        else if(lhs_type.type_name != rhs_type.type_name.substr(4) && lhs_type.type_name.substr(1) != rhs_type.type_name.substr(4)){ //not equal types and without pointer not equal
            errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": assignment lhs has type " + lhs_type.type_name + " but we're allocating type " + rhs_type.type_name.substr(4));
            tf = false;
         }     
    }
    return tf;
}

bool If::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    TypeName exp_type (guard->typeCheck(gamma, fun, errors));
    bool tf = true;
    if((exp_type.type_name != "int") && (exp_type.type_name != "_")) {
        errors.push_back("[IF] in function " + fun->name + ": if guard has type " + exp_type.type_name + " instead of int");
        tf = false;
    }
    for(auto s: tt){ s->typeCheck(gamma, fun, loop, errors); }
    for(auto s: ff){ s->typeCheck(gamma, fun, loop, errors); }
    return tf;
}
bool While::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    TypeName exp_type (guard->typeCheck(gamma, fun, errors));
    if((exp_type.type_name != "int") && (exp_type.type_name != "_")) {
        errors.push_back("[WHILE] in function " + fun->name + ": while guard has type " + exp_type.type_name + " instead of int");
    }
    for(auto s: body){ s->typeCheck(gamma, fun, true, errors); } 
    return true;
}

bool global_TC(Gamma& gamma, Errors& errors, string global_name, Type* type) {
    bool tf = true;
    TypeName global_type = type->typeName();
    if ( global_type.isStruct() || global_type.isNonPointerFunction() ) { 
        errors.push_back("[GLOBAL] global " + global_name + " has a struct or function type");
        tf = false; 
    }
    return tf;
}

bool Struct::typeCheck(Gamma& gamma, Errors& errors) const {
    bool tf = true;
    for (Decl* decl: fields) {
        TypeName field_type = decl->typeName();
        if ( field_type.isStruct() || field_type.isNonPointerFunction() ) { 
            errors.push_back("[STRUCT] struct " + name + " field " + decl->name + " has a struct or function type");
            tf = false; 
        }
    }
    return tf;
}

bool Function::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    //Need to check for bad parameter type
    bool tf = true;
    for (Decl* decl: params) {
        TypeName var_type = decl->typeName();
        if ( var_type.isStruct() || var_type.isNonPointerFunction() ) { 
            errors.push_back("[FUNCTION] in function " + name + ": variable " + decl->name + " has a struct or function type");
            tf = false; 
        }
    }
    for (auto [decl, exp]: locals) {
        TypeName var_type = decl->typeName();
        if ( var_type.isStruct() || var_type.isNonPointerFunction() ) { 
            errors.push_back("[FUNCTION] in function " + name + ": variable " + decl->name+ " has a struct or function type");
            tf = false; 
        }
        TypeName exp_type = exp->typeCheck(gamma, fun, errors);
        if (var_type != exp_type) {
            errors.push_back("[FUNCTION] in function " + name + ": variable " + decl->name + " with type " + var_type.type_name + " has initializer of type " + exp_type.type_name);
            tf = false; 
        }
    }
    for (Stmt* stmt : stmts) {
        if (!stmt->typeCheck(gamma, fun, false, errors)) { tf = false; }
    }
    return tf;
}

bool Program::typeCheck(Gamma& gamma, Errors& errors, unordered_map<string, Gamma>& locals_map) const { //input gamma is the initial gamma
    bool tf = true;
    for (Decl* decl: globals) {
        if (!global_TC(gamma, errors, decl->name, decl->type)) { tf = false; }
    }
    for (Struct* str: structs) {
        if (!str->typeCheck(gamma, errors)) { tf = false; }
    }
    for (Function* fun: functions) { //Creating locals map
        if (!fun->typeCheck(locals_map[fun->name], fun, errors)) { tf = false; }
    } 
    return tf;
}


//LIR
//Helper function for le (lowering lvals as expressions)
tuple<unsigned int, string, string> le(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, Lval* lval, map<string, string>& extern_map, map<string, string>& function_map) {
    auto [eval_var_src, eval_string_src, op_type] = lval->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
    if (eval_var_src == 0) { return make_tuple(eval_var_src, eval_string_src, op_type); } //else, lv!=Id(name)
    string res = "";
    string lhs_type;
    res += eval_string_src; //building the res instructions
    eval_string_src = "_t" + to_string(eval_var_src);
    lhs_type = op_type.substr(1);
    //Create fresh type of type t std src:&t
    tempsToType.push_back(lhs_type);
    unsigned int fresh_var = tempsToType.size();
    res += "    Load(_t" + to_string(fresh_var) + ", " + eval_string_src + ")\n";  
    return make_tuple(fresh_var, res, lhs_type); //returning new type
}
tuple<unsigned int, string, string> LvalDeref::lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const {
    return le(tempsToType, numLabels, gamma, fun, lval, extern_map, function_map);
}
tuple<unsigned int, string, string> LvalArrayAccess::lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const {
    auto [eval_var_src, eval_string_src, src_type] = le(tempsToType, numLabels, gamma, fun, ptr, extern_map, function_map);
    auto [eval_var_idx, eval_string_idx, _] = index->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
    string res = "";
    string lhs_type = "";
    if (eval_var_src != 0) {
        res += eval_string_src;
        eval_string_src = "_t" + to_string(eval_var_src);
    }
    if (eval_var_idx != 0) {
        res += eval_string_idx;
        eval_string_idx = "_t" + to_string(eval_var_idx);
    }
    lhs_type = src_type;
    tempsToType.push_back(lhs_type);
    unsigned int fresh_var_lhs = tempsToType.size();
    res += "    Gep(_t" + to_string(fresh_var_lhs) + ", " + eval_string_src + ", " + eval_string_idx + ")\n";
    return make_tuple(fresh_var_lhs, res, lhs_type);
}

tuple<unsigned int, string, string> ExpFieldAccess::lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const {
        auto [eval_var_src, eval_string_src, src_type] = ptr->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        string res = "";
        string lhs_type = "";
        if (eval_var_src != 0) {
            res += eval_string_src;
            eval_string_src = "_t" + to_string(eval_var_src);
            src_type = tempsToType[eval_var_src-1];
        }
        size_t start_pos = src_type.find("Struct(") + 7;
        string struct_type = src_type.substr(start_pos, src_type.find(')') - start_pos);
        string field_type = delta[struct_type][field]->toLIRType();
        tempsToType.push_back("Ptr(" + field_type + ")");
        unsigned int fresh_var_fldp = tempsToType.size();
        lhs_type = field_type;
        tempsToType.push_back(lhs_type);
        unsigned int fresh_var_lhs = tempsToType.size();
        res += "    Gfp(_t" + to_string(fresh_var_fldp) + ", " + eval_string_src + ", " + field + ")\n";
        res += "    Load(_t" + to_string(fresh_var_lhs) + ", _t" + to_string(fresh_var_fldp) + ")\n";
        return make_tuple(fresh_var_lhs, res, lhs_type);
}

tuple<unsigned int, string, string> LvalFieldAccess::lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const {
    auto [eval_var_src, eval_string_src, src_type] = le(tempsToType, numLabels, gamma, fun, ptr, extern_map, function_map);
    string res = "";
    string lhs_type = "";
    if (eval_var_src != 0) {
        res += eval_string_src;
        eval_string_src = "_t" + to_string(eval_var_src);
    }
    string struct_type = src_type.substr(1);    
    string field_type = delta[struct_type][field]->toLIRType();
    lhs_type = "Ptr(" + field_type + ")";
    tempsToType.push_back(lhs_type);
    unsigned int fresh_var_lhs = tempsToType.size();
    res += "    Gfp(_t" + to_string(fresh_var_lhs) + ", " + eval_string_src + ", " + field + ")\n";
    return make_tuple(fresh_var_lhs, res, lhs_type);
}
tuple<unsigned int, string, string> ExpCall::lower(vector<string>& tempsToType, unsigned int& numLabels, Gamma& gamma, const Function* fun, map<string, string>& extern_map, map<string, string>& function_map) const {
    string res = "";
    string lhs_type = "";
    string aops = "[";
    for (unsigned int i = 0; i < args.size(); i++) {
        auto [eval_var_src, eval_string_src, src_type] = args[i]->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        if (eval_var_src != 0) {
            res += eval_string_src;
            eval_string_src = "_t" + to_string(eval_var_src);
        }
        aops += eval_string_src;
        if (i != args.size() - 1) { aops += ", "; }
    }
    aops += "]";

    ParamsReturnVal prv;
    string callee_name;
    bool direct = true;
    bool isExtern = false;
    bool isFunction = false;

    if ( callee->isFieldAccess() ) { //can assume struct name exists, not field
        Errors errors;
        string struct_name = callee->getPtr()->typeCheck(gamma, fun, errors).type_name.substr(1);
        string field_name = callee->getName();
        auto [eval_var_lhs, eval_string_lhs, _] = callee->lower(tempsToType, numLabels, gamma, fun, extern_map, function_map);
        if (eval_var_lhs != 0) {
            res += eval_string_lhs;
            eval_string_lhs = "_t" + to_string(eval_var_lhs);
        }
        prv = struct_functions_map[struct_name][field_name];
        callee_name = eval_string_lhs; //id is the most recent fresh variable made
    } else { //can assume it exists since we already returned for values not in gamma
        callee_name = callee->getName();
        // cout << "Callee name: " << callee_name << "\n";
        prv = functions_map[callee_name]; 
        // cout << "Callee type: " << callee_type << "\n";
        if (!callee->isId()) {
            direct = false;
        } else {
            for (Decl* param: fun->params) { if (param->name == callee_name) { direct = false; } }
            for (auto [local, _]: fun->locals) { if (local->name == callee_name) { direct = false; } }
        }
        for (auto& pair: extern_map) { if (pair.first == callee_name) isExtern = true; }
        for (auto& pair: function_map) { if (pair.first == callee_name) isFunction = true; }
        // cout << "For function call " << callee_name << " is it direct? " << direct << "\n";
    }
    lhs_type = prv.second->toLIRType();
    tempsToType.push_back(lhs_type);
    unsigned int fresh_var_lhs = tempsToType.size();

    if (direct && isExtern) {
        res += "    CallEx(_t" + to_string(fresh_var_lhs) + ", " + callee_name + ", " + aops + ")\n";
    } else {
        unsigned int NEXT = numLabels + 1;
        numLabels++;
        if (direct && isFunction) {
            res += "    CallDirect(_t" + to_string(fresh_var_lhs) + ", " + callee_name + ", " + aops + ", lbl" + to_string(NEXT) + ")\n";
        } else {
            res += "    CallIndirect(_t" + to_string(fresh_var_lhs) + ", " + callee_name + ", " + aops + ", lbl" + to_string(NEXT) + ")\n";
        }
        res += "\n  lbl" + to_string(NEXT) + ":\n";
    }
    return make_tuple(fresh_var_lhs, res, lhs_type); 
}
#endif