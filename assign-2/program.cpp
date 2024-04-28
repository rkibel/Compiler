#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>

// class typeFail : std::exception {
//     private: 
//     std::string error_message;
//     public:
//     typeFail(std::string message) : error_message(message) {}
//     std::string get() const { return error_message; }
// };

struct TypeName {
    std::string type_name;
    TypeName( std::string type_name) : type_name(type_name) {}; //No default arg to ensure constructing correctly
    TypeName(const TypeName& other) : type_name(other.type_name) {}
    TypeName& operator=(const TypeName& other) {
        if (this != &other) {
            type_name = other.type_name;
        }
        return *this;
    }
    bool operator==(const TypeName& other) const {
        try {
            //Checks for equality to Any
            if (type_name == "_" || other.type_name == "_") { return true; } //Any equal to others
            if (type_name == other.type_name) { return true; } //If equal then return true
            //Checks for null pointer equality to other pointers
            return type_name.at(0) == '&' && other.type_name.at(0) == '&' && (type_name.at(1) == '_' || other.type_name.at(1) == '_');
        } catch (const std::out_of_range& e) {} 
        return false;
    }
};
struct Exp;

using Gamma = std::unordered_map<std::string, TypeName*>;
using Delta = std::unordered_map<std::string, Gamma>;
using Errors = std::map<std::string, std::vector<std::string>>;

struct Type {
    virtual void print(std::ostream& os) const {}
    virtual ~Type() {}
    virtual TypeName typeName() const { return TypeName("_"); }
    friend std::ostream& operator<<(std::ostream& os, const Type& obj) {
        obj.print(os);
        return os;
    }
};
struct Int : Type {
    void print(std::ostream& os) const override {
        os << "Int";
    }
    TypeName typeName() const override {
        return TypeName("int");
    }
};
struct StructType : Type {
    std::string name;
    void print(std::ostream& os) const override { 
        os << "Struct(" << name << ")"; 
    }
    TypeName typeName() const override {
        return TypeName("struct_" + name);
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
    TypeName typeName() const override {
        std::string prms_type_names;
        for (unsigned int i = 0; i < prms.size(); i++) {
            prms_type_names += prms[i]->typeName().type_name;
            if (i != prms.size() - 1) prms_type_names += ", ";
        }
        return TypeName("(" + prms_type_names + ") -> " + ret->typeName().type_name);
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
    ~Ptr() { delete ref; }
};
struct Any : Type {
    void print(std::ostream& os) const override { 
        os << "_";
    }
    TypeName typeName() const override {
        return TypeName("_");
    }
};

struct UnaryOp {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* exp) const { return TypeName("_"); };
    friend std::ostream& operator<<(std::ostream& os, const UnaryOp& uo) {
        uo.print(os);
        return os;
    }
};
struct Neg : UnaryOp {
    void print(std::ostream& os) const override { os << "Neg"; }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* operand) const override; //Defined at the bottom
};
struct UnaryDeref : UnaryOp {
    void print(std::ostream& os) const override { os << "Deref"; }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* operand) const override;
};

struct BinaryOp{
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* left, Exp* right) const; //define for rest, overload for eq/not eq (defined below)
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

struct Exp {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name) const { return TypeName("_"); };
    virtual ~Exp() {}
    friend std::ostream& operator<<(std::ostream& os, const Exp& exp) {
        exp.print(os);
        return os;
    }
};
struct Num : Exp {
    int32_t n;
    void print(std::ostream& os) const override { os << "Num(" << n << ")"; }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name) const override { return TypeName("int"); };
};
struct ExpId : Exp {
    std::string name;
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name) const override {
        try {
            TypeName* temp = gamma.at(name); //Returns TypeName struct
            return *temp;
        } catch (const std::out_of_range& e) {
            errors["[ID]"].push_back("[ID] in function " + function_name + ": variable " + name + " undefined");
            gamma[name] = new TypeName("_"); //this should fix the types for expression
            return TypeName("_");
        }
    };
};
struct Nil : Exp {
    void print(std::ostream& os) const override { os << "Nil"; }
    TypeName typeName() const {
        return TypeName("&_");
    }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name) const override {
        return TypeName("&_");
    };
};
struct UnOp : Exp {
    UnaryOp* op;
    Exp* operand;
    void print(std::ostream& os) const override { os << *op << "(" << *operand << ")"; }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name) const override {
        return op->typeCheck(gamma, delta, errors, function_name, operand);
    };
    ~UnOp() { delete operand; }
};
struct BinOp : Exp {
    BinaryOp* op;
    Exp* left;
    Exp* right;
    void print(std::ostream& os) const override {
        os << "BinOp(\nop = " << *op << ",\nleft = " << *left << ",\nright = " << *right << "\n)"; 
    }
    TypeName typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name) const override {
        return op->typeCheck(gamma, delta, errors, function_name, left, right);
    };
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
        }
        os << "\n]\n)";
    }
    ~ExpCall() {
        delete callee;
        for (Exp* exp: args) delete exp;
    }
};
struct AnyExp : Exp {
    void print(std::ostream& os) const override { 
        os << "_";
    }
    TypeName typeName() const {
        return TypeName("_");
    }
};

struct Lval {
    virtual void print(std::ostream& os) const {}
    virtual ~Lval() {}
    friend std::ostream& operator<<(std::ostream& os, const Lval& lval) {
        lval.print(os);
        return os;
    }
};
struct LvalId : Lval {
    std::string name;
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
};
struct LvalDeref : Lval {
    Lval* lval;
    void print(std::ostream& os) const override { os << "Deref(" << *lval << ")"; }
    ~LvalDeref() { delete lval; }
};
struct LvalArrayAccess : Lval {
    Lval* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    ~LvalArrayAccess() { delete ptr; delete index; }
};
struct LvalFieldAccess : Lval {
    Lval* ptr;
    std::string field;
    void print(std::ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    ~LvalFieldAccess() { delete ptr; }
};


struct Rhs {
    virtual void print(std::ostream& os) const {}
    virtual ~Rhs() {}
    friend std::ostream& operator<<(std::ostream& os, const Rhs& rhs) {
        rhs.print(os);
        return os;
    }
};
struct RhsExp : Rhs {
    Exp* exp;
    void print(std::ostream& os) const override { os << *exp; }
    ~RhsExp() { delete exp; }
};
struct New : Rhs {
    Type* type;
    Exp* amount;
    void print(std::ostream& os) const override { os << "New(" << *type << ", " << *amount << ")"; }
    ~New() { delete type; delete amount; }
};

struct Stmt {
    virtual void print(std::ostream& os) const {}
    virtual bool typeCheck(Gamma& gamma, Delta& delta, TypeName return_type, bool loop, Errors& errors, std::string function_name) const { return true; };
    virtual ~Stmt() {}
    friend std::ostream& operator<<(std::ostream& os, const Stmt& stmt) {
        stmt.print(os);
        return os;
    }
};
struct Break : Stmt {
    void print(std::ostream& os) const override { os << "Break"; }
};
struct Continue : Stmt {
    void print(std::ostream& os) const override { os << "Continue"; }
    // typeCheck(......bool loop, ....) {
    //     typeCheck(....loop....)
    // }
};
struct Return : Stmt {
    // WARNING: if there is no return, exp is AnyExp : Exp
    Exp* exp;
    void print(std::ostream& os) const override {
        os << "Return(" << *exp << ")";
    }
    bool typeCheck(Gamma& gamma, Delta& delta, TypeName return_type, bool loop, Errors& errors, std::string function_name) const override {
        TypeName exp_type ( exp->typeCheck(gamma, delta, errors, function_name) ); //store typename given, typeCheck will replace undefined variables with Any
        if ( !(exp_type == return_type)) { //If not equal, then at least one of the types not _ and they are different
            if ( return_type.type_name == "_") {
                errors["[RETURN-1]"].push_back("[RETURN-1] in function " + function_name + ": should return nothing but returning " + exp_type.type_name);
            } else if ( exp_type.type_name == "_") {
                errors["[RETURN-2]"].push_back("[RETURN-2] in function " + function_name + ": should return " + return_type.type_name + " but returning nothing");
            } else {
                errors["[RETURN-2]"].push_back("[RETURN-2] in function " + function_name + ": should return " + return_type.type_name + " but returning " + exp_type.type_name);
            }
            return false;
        }
        return true;
    };
    ~Return() { delete exp; }
};
struct Assign : Stmt {
    Lval* lhs;
    Rhs* rhs;
    void print(std::ostream& os) const override {
        os << "Assign(\nlhs = " << *lhs << ",\nrhs = " << *rhs << "\n)";
    }
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
    // typeCheck(....., true,  ...)
    ~While() { delete guard; for (Stmt* stmt: body) delete stmt; }
};

struct Decl {
    //WARNING: should only call it's own typename
    std::string name;
    Type* type;
    ~Decl() { delete type; }
    friend std::ostream& operator<<(std::ostream& os, const Decl& decl) {
        os << "Decl(" << decl.name << ", " << *(decl.type) << ")";
        return os;
    }
    TypeName typeName() const { 
        return type->typeName();
    }
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
};

struct Function {
    std::string name;
    std::vector<Decl*> params;
    // WARNING: if function has no return type, rettyp is Any : Type
    Type* rettyp;
    // WARNING: for each local, if there is no value after declaration, Exp is AnyExp : Exp
    std::vector<std::pair<Decl*, Exp*>> locals;
    std::vector<Stmt*> stmts;
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
};

//Definitions of typeCheck which rely on types defined later on

TypeName Neg::typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* operand) const {
    TypeName exp_type ( operand->typeCheck(gamma, delta, errors, function_name) );
    if (exp_type.type_name != "int") {
        errors["[NEG]"].push_back("[NEG] in function " + function_name + ": negating type " + exp_type.type_name + " instead of int");
    }
    return TypeName("int");
};

TypeName UnaryDeref::typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* operand) const {
    TypeName exp_type ( operand->typeCheck(gamma, delta, errors, function_name) );
    if (exp_type.type_name == "_") { return exp_type; }
    if (exp_type.type_name != "&_" && exp_type.type_name != "_" && exp_type.type_name[0] != '&') {
        errors["[NEG]"].push_back("[NEG] in function " + function_name + ": dereferencing type " + exp_type.type_name + " instead of pointer");
        return TypeName("_");
    }
    return TypeName( exp_type.type_name.substr(1) );
}

TypeName BinaryOp::typeCheck(Gamma& gamma, Delta& delta, Errors& errors, std::string function_name, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, delta, errors, function_name) );
    TypeName right_type ( right->typeCheck(gamma, delta, errors, function_name) );
    if (left_type.type_name != "_" && left_type.type_name != "int") {
        errors["[BINOP-REST]"].push_back("[BINOP-REST] in function " + function_name + ": operand has type " + left_type.type_name + " instead of int");
    }
    if (right_type.type_name != "_" && right_type.type_name != "int") {
        errors["[BINOP-REST]"].push_back("[BINOP-REST] in function " + function_name + ": operand has type " + right_type.type_name + " instead of int");
    }
    return TypeName("int");

}