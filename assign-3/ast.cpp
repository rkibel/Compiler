#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>

#ifndef AST_CPP
#define AST_CPP

struct TypeName {
    std::string type_name;
    std::string struct_name; // used to pass in for exp or *
    std::string field_name;  // used to pass in for exp or *
    TypeName( std::string t, std::string s = "", std::string f = "") : type_name(t), struct_name(s), field_name(f) {};
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
        } catch (const std::out_of_range& e) {} 
        return false;
    }
    bool operator!=(const TypeName& other) const {
        return !(*this == other);
    }
    bool isFunction() const { return type_name[0] == '(' || type_name.substr(0,2) == "&("; }
    bool isNonPointerFunction() const { return type_name[0] == '('; }
    bool isStruct() const { return type_name[0] != '&' && type_name.find('(') == std::string::npos && type_name.substr(0,3) != "int"; }
    bool isValidFieldAcesss() const { return type_name[0] == '&' && type_name[1] != '(' && type_name[1] != '&' && type_name.substr(0,5) != "&int"; }
    bool isPointerToFunction() const { return type_name[0] == '(' || type_name.find("&(") != std::string::npos; }
};

namespace AST {

struct Exp;
struct Type;
struct Function;

using ParamsReturnVal = std::pair<std::vector<Type*>, Type*>;
using Gamma = std::unordered_map<std::string, TypeName*>;
using Delta = std::unordered_map<std::string, Gamma>;
using Errors = std::vector<std::string>;
using FunctionsInfo = std::unordered_map<std::string, ParamsReturnVal>;
using StructFunctionsInfo = std::unordered_map<std::string, FunctionsInfo>;

struct Type {
    virtual void print(std::ostream& os) const {}
    virtual bool isAny() const { return false; }
    virtual TypeName typeName() const { return TypeName("_"); }
    virtual ParamsReturnVal funcInfo() const;
    virtual ~Type() {}
    friend std::ostream& operator<<(std::ostream& os, const Type& obj) {
        obj.print(os);
        return os;
    }
    virtual std::string toLIRString() const { return ""; }
};
struct Int : Type {
    void print(std::ostream& os) const override {
        os << "Int";
    }
    TypeName typeName() const override {
        return TypeName("int");
    }
    std::string toLIRString() const override { return "Int"; }
};
struct StructType : Type {
    std::string name;
    void print(std::ostream& os) const override { 
        os << "Struct(" << name << ")"; 
    }
    TypeName typeName() const override {
        return TypeName(name);
    }
    std::string toLIRString() const override { return "Struct(" + name + ")"; }
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
    TypeName typeName() const override {
        std::string prms_type_names;
        for (unsigned int i = 0; i < prms.size(); i++) {
            prms_type_names += prms[i]->typeName().type_name;
            if (i != prms.size() - 1) prms_type_names += ", ";
        }
        std::string temp = "(" + prms_type_names + ") -> " + ret->typeName().type_name;
        return TypeName(temp);
    }
    ParamsReturnVal funcInfo () const override { return std::make_pair(prms, ret); }
    ~Fn() {
        for (Type* t: prms) delete t;
        delete ret;
    }
    std::string toLIRString() const override { 
        std::string res = "Fn([";
        for (unsigned int i = 0; i < prms.size(); i++) {
            res += prms[i]->toLIRString();
            if (i != prms.size() - 1) res += ", ";
        }
        res += "], " + ret->toLIRString() + ")";
        return res;
    }
};
struct Ptr : Type {
    Type* ref;
    void print(std::ostream& os) const override { 
        os << "Ptr(" << *ref << ")";
    }
    TypeName typeName() const override {
        std::string ptr_type_names;
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
    std::string toLIRString() const override {  return "Ptr(" + ref->toLIRString() + ")"; }
};
struct Any : Type {
    void print(std::ostream& os) const override { 
        os << "_";
    }
    TypeName typeName() const override {
        return TypeName("_");
    }
    bool isAny() const override { return true; }
    std::string toLIRString() const override { return "_"; }
};

struct UnaryOp {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* exp) const { return TypeName("_"); };
    friend std::ostream& operator<<(std::ostream& os, const UnaryOp& uo) {
        uo.print(os);
        return os;
    }
};
struct Neg : UnaryOp {
    void print(std::ostream& os) const override { os << "Neg"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const override; //Defined at the bottom
};
struct UnaryDeref : UnaryOp {
    void print(std::ostream& os) const override { os << "Deref"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const override;
};

struct BinaryOp{
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const; //define for rest, overload for eq/not eq (defined below)
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
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const override;
};
struct NotEq : Equal{ //so the same typeCheck function is inherited
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

struct Exp {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual std::string getName() { return "_"; };
    virtual bool isAny() const { return false; }
    virtual bool isFieldAccess() const { return false; }
    virtual ~Exp() {}
    friend std::ostream& operator<<(std::ostream& os, const Exp& exp) {
        exp.print(os);
        return os;
    }
};
struct Num : Exp {
    int32_t n;
    void print(std::ostream& os) const override { os << "Num(" << n << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override { return TypeName("int"); };
    std::string getName() override { return "int"; }
};
struct ExpId : Exp {
    std::string name;
    ExpId() {}
    ExpId(const std::string& name) : name(name) {}
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { return name; }
};
struct Nil : Exp {
    void print(std::ostream& os) const override { os << "Nil"; }
    TypeName typeName() const { return TypeName("&_"); }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override { return TypeName("&_");};
    std::string getName() override { return "_"; }
};
struct UnOp : Exp {
    UnaryOp* op;
    Exp* operand;
    void print(std::ostream& os) const override { os << *op << "(" << *operand << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, operand);
    }
    std::string getName() override { return operand->getName(); }
    ~UnOp() { delete operand; }
};
struct BinOp : Exp {
    BinaryOp* op;
    Exp* left;
    Exp* right;
    void print(std::ostream& os) const override {
        os << "BinOp(\nop = " << *op << ",\nleft = " << *left << ",\nright = " << *right << "\n)"; 
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, left, right);
    }
    std::string getName() override { return "_"; }
    ~BinOp() { delete left; delete right; }
};
struct ExpArrayAccess : Exp {
    Exp* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { return ptr->getName(); }
    ~ExpArrayAccess() { delete ptr; delete index; }
};
struct ExpFieldAccess : Exp {
    Exp* ptr;
    std::string field;
    void print(std::ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    bool isFieldAccess() const override { return true; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { return field; }
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
        }
        os << "\n]\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~ExpCall() {
        delete callee;
        for (Exp* exp: args) delete exp;
    }
    std::string getName() override { return callee->getName(); }
};
struct AnyExp : Exp {
    void print(std::ostream& os) const override { os << "_"; }
    TypeName typeName() const { return TypeName("_"); }
    bool isAny() const override { return true; }
    std::string getName() override { return "_"; }
};

struct Lval {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual std::string getName() { return "_"; }
    virtual bool isFieldAccess() const { return false; }
    virtual ~Lval() {}
    friend std::ostream& operator<<(std::ostream& os, const Lval& lval) {
        lval.print(os);
        return os;
    }
};
struct LvalId : Lval {
    std::string name;
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { return name; }
};
struct LvalDeref : Lval {
    Lval* lval;
    void print(std::ostream& os) const override { os << "Deref(" << *lval << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { return lval->getName(); }
    ~LvalDeref() { delete lval; }
};
struct LvalArrayAccess : Lval {
    Lval* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { return ptr->getName(); }
    ~LvalArrayAccess() { delete ptr; delete index; }
};
struct LvalFieldAccess : Lval {
    Lval* ptr;
    std::string field;
    void print(std::ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    std::string getName() override { return ptr->getName(); }
    bool isFieldAccess() const override { return false;  }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~LvalFieldAccess() { delete ptr; }
};


struct Rhs {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual ~Rhs() {}
    friend std::ostream& operator<<(std::ostream& os, const Rhs& rhs) {
        rhs.print(os);
        return os;
    }
};
struct RhsExp : Rhs {
    Exp* exp;
    void print(std::ostream& os) const override { os << *exp; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~RhsExp() { delete exp; }
};
struct New : Rhs {
    Type* type;
    Exp* amount;
    void print(std::ostream& os) const override { os << "New(" << *type << ", " << *amount << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~New() { delete type; delete amount; }
};

struct Stmt {
    virtual void print(std::ostream& os) const {}
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const { return true; };
    virtual std::string getName() { return "_"; }
    virtual ~Stmt() {}
    friend std::ostream& operator<<(std::ostream& os, const Stmt& stmt) {
        stmt.print(os);
        return os;
    }
};
struct Break : Stmt {
    void print(std::ostream& os) const override { os << "Break"; }
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
};
struct Continue : Stmt {
    void print(std::ostream& os) const override { os << "Continue"; }
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
};
struct Return : Stmt {
    // WARNING: if there is no return, exp is AnyExp : Exp
    Exp* exp;
    void print(std::ostream& os) const override {
        os << "Return(" << *exp << ")";
    }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~Return() { delete exp; }
};
struct Assign : Stmt {
    Lval* lhs;
    Rhs* rhs;
    void print(std::ostream& os) const override {
        os << "Assign(\nlhs = " << *lhs << ",\nrhs = " << *rhs << "\n)";
    }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~Assign() { delete lhs; delete rhs; }
};
struct StmtCall : Stmt {
    Lval* callee;
    std::vector<Exp*> args;
    void print(std::ostream& os) const override {
        os << "Call(\ncallee = " << *callee << ",\nargs = [";
        for (unsigned int i = 0; i < args.size(); i++) {
            os << *(args[i]);
            if (i != args.size() - 1) os << ", ";
        }
        os << "]\n)";
    }
    std::string getName() override { return callee->getName(); }
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
    ~StmtCall() { delete callee; for (Exp* exp: args) delete exp; }
};
struct If : Stmt {
    Exp* guard;
    std::vector<Stmt*> tt;
    std::vector<Stmt*> ff;
    void print(std::ostream& os) const override {
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
    std::vector<Stmt*> body;
    void print(std::ostream& os) const override {
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
    std::string name;
    Type* type; // Fn type if this Decl is an extern
    std::vector<Decl*> params; // optional, used to keep track of parameters for extern decl
    ~Decl() { delete type; }
    friend std::ostream& operator<<(std::ostream& os, const Decl& decl) {
        os << "Decl(" << decl.name << ", " << *(decl.type) << ")";
        return os;
    }
    TypeName typeName() const { return type->typeName(); }
    ParamsReturnVal funcInfo () const { return type->funcInfo(); }
};

struct Struct {
    std::string name;
    std::vector<Decl*> fields;
    ~Struct() { for (Decl* decl: fields) delete decl; }
    friend std::ostream& operator<<(std::ostream& os, const Struct& str) {
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
    std::string name;
    std::vector<Decl*> params;
    // WARNING: if function has no return type, rettyp is Any : Type
    Type* rettyp;
    // WARNING: for each local, if there is no value after declaration, Exp is AnyExp : Exp
    std::vector<std::pair<Decl*, Exp*>> locals;
    std::vector<Stmt*> stmts;
    std::string toLIRString() const {
        std::string res = "Fn([";
        for (unsigned int i = 0; i < params.size(); i++) {
            res += params[i]->type->toLIRString();
            if (i != params.size() - 1) res += ", ";
        }
        res += "], " + rettyp->toLIRString() + ")";
        return res;
    }
    ~Function() {
        for (Decl* decl: params) delete decl;
        delete rettyp;
        for (auto [decl, exp]: locals) { delete decl; delete exp; }
        for (Stmt* stmt: stmts) delete stmt;
    }
    friend std::ostream& operator<<(std::ostream& os, const Function& func) {
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
        std::string prms_type_names;
        for (unsigned int i = 0; i < params.size(); i++) {
            prms_type_names += params[i]->typeName().type_name;
            if (i != params.size() - 1) prms_type_names += ", ";
        }
        return TypeName("&(" + prms_type_names + ") -> " + rettyp->typeName().type_name); //Functions should return a pointer to a function
    }
    ParamsReturnVal funcInfo () const { 
        std::vector<Type*> prms;
        for (Decl* decl: params) {
            prms.push_back(decl->type);
        }
        return std::make_pair(prms, rettyp); 
    }
    bool typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const;
};

struct Program {
    std::vector<Decl*> globals;
    std::vector<Struct*> structs;
    std::vector<Decl*> externs;
    std::vector<Function*> functions;
    ~Program() { 
        for (Decl* decl: globals) delete decl;
        for (Struct* str: structs) delete str;
        for (Decl* decl: externs) delete decl;
        for (Function* function: functions) delete function;
    }
    friend std::ostream& operator<<(std::ostream& os, const Program& prog) {
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
    bool typeCheck(Gamma& gamma, Errors& errors, std::unordered_map<std::string, Gamma>& locals_map) const;
};

}

using namespace AST;
using ParamsReturnVal = std::pair<std::vector<Type*>, Type*>;
using Gamma = std::unordered_map<std::string, TypeName*>;
using Delta = std::unordered_map<std::string, Gamma>;
using Errors = std::vector<std::string>;
using FunctionsInfo = std::unordered_map<std::string, ParamsReturnVal>;
using StructFunctionsInfo = std::unordered_map<std::string, FunctionsInfo>;

extern Delta delta;
extern FunctionsInfo functions_map;
extern StructFunctionsInfo struct_functions_map;

// General functions (with _TC for ones used by both Exp and Stmt) which depend on earlier definitions
ParamsReturnVal Type::funcInfo () const {
    return std::make_pair(std::vector<Type*>(), new Any());
}

TypeName RhsExp::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return exp->typeCheck(gamma, fun, errors);
}

TypeName const id_TC(Gamma& gamma, const Function* fun, Errors& errors, std::string name) {
    try {
        TypeName* temp = gamma.at(name); //Returns TypeName* struct
        return *temp;
    } catch (const std::out_of_range& e) {
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

TypeName const deref_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> operand) {
    TypeName exp_type = std::visit([&exp_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, operand);
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

TypeName const arrayAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> ptr, Exp* index) {
    TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
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

TypeName const fieldAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> ptr, std::string field) {
    TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    if (ptr_type.type_name == "_") { return TypeName("_"); } //errors won't happpen given Any struct
    
    std::string struct_type = ptr_type.type_name.substr(1);    
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
    return TypeName((*(delta[struct_type][field])).type_name, struct_type, field);
}
TypeName ExpFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName temp = fieldAccess_TC(gamma, fun, errors, ptr, field);
    return temp;
}
TypeName LvalFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName temp = fieldAccess_TC(gamma, fun, errors, ptr, field);
    return temp;
}

std::pair<TypeName, bool> call_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> callee, std::vector<Exp*> arg) {    
    TypeName callee_type = std::visit([&callee_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, callee);
    bool isFieldAccess = std::visit([](auto* arg) { return arg->isFieldAccess(); }, callee);
    std::string callee_name = std::visit([](auto* arg) { return arg->getName(); }, callee); //field name for field, function for others
    
    if (callee_name == "_") { return std::pair<TypeName, bool>(TypeName("_"), true); } //no need to continue if Any
    bool success = true;
    
    std::string expression_statement = std::holds_alternative<Exp*>(callee) ? "[ECALL" : "[SCALL";
    if (callee_name == "main") { //If so, can't be an extern call
        errors.push_back(expression_statement+"-INTERNAL] in function " + fun->name + ": calling main");
        success = false;
    }
    if (callee_type.type_name == "_") { return std::pair<TypeName, bool>(TypeName("_"), true); }
    if (!callee_type.isFunction()) { //main can be a parameter, so should check if bad type
        errors.push_back(expression_statement+"-*] in function " + fun->name + ": calling non-function type " + callee_type.type_name);
        return std::pair<TypeName, bool>(TypeName("_"), false);
    }
    if (callee_name == "main" && functions_map.find("main") == functions_map.end()) { //if main not defined as a function, return
            return std::pair<TypeName, bool>(TypeName("_"), false);
    }
    if (callee_type.type_name == "_") { return std::pair<TypeName, bool>(TypeName("_"), true); } //callee is just any, just return undefined error which typeCheck already added (and possibly main error)
    
    ParamsReturnVal prv;
    
    if ( isFieldAccess ) { //can assume struct name exists, not field
        std::string struct_name = callee_type.struct_name;
        std::string field_name = callee_type.field_name;
        prv = struct_functions_map[struct_name][field_name];
    } else { //can assume it exists since we already returned for values not in gamma
        prv = functions_map[callee_name]; 
    }
    
    std::string internal_external = ( callee_type.type_name[0] == '&' ) ? "-INTERNAL]" : "-EXTERN]";
    std::string error_type = expression_statement + internal_external;
    if (expression_statement != "[SCALL" && prv.second->typeName().type_name == "_") { // empty return type
        errors.push_back(error_type + " in function " + fun->name + ": calling a function with no return value");
        success = false;
    }
    if (prv.first.size() != arg.size()) {
        errors.push_back(error_type + " in function " + fun->name + 
        ": call number of arguments (" + std::to_string(arg.size()) + ") and parameters (" + std::to_string(prv.first.size()) + ") don't match");
        success = false;
    }
    unsigned int min = (prv.first.size() < arg.size()) ? prv.first.size() : arg.size();
    for (unsigned int i = 0; i < min; i++) {
        TypeName param_type = prv.first[i]->typeName();
        TypeName arg_type = arg[i]->typeCheck(gamma, fun, errors);
        if (param_type != arg_type) {
            errors.push_back(error_type + " in function " + fun->name + ": call argument has type " + arg_type.type_name 
                + " but parameter has type " + param_type.type_name);
            success = false;
        }
    }
    std::pair<TypeName, bool> temp(prv.second->typeName(), success);
    return std::pair<TypeName, bool>(prv.second->typeName(), success);
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

bool global_TC(Gamma& gamma, Errors& errors, std::string global_name, Type* type) {
    bool tf = true;
    TypeName global_type = type->typeName();
    if ( global_type.isStruct() || global_type.isNonPointerFunction() ) { 
        errors.push_back("[GLOBAL] global " + global_name + " has a struct or function type");
        tf = false; 
    }
    return tf;
}

bool Struct::typeCheck(Gamma& gamma, Errors& errors) const {
    extern Delta delta;
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

bool Program::typeCheck(Gamma& gamma, Errors& errors, std::unordered_map<std::string, Gamma>& locals_map) const { //input gamma is the initial gamma
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

#endif