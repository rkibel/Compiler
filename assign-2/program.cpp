#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>

class TypeName {
    private:
        std::string type_name;
    public:
        TypeName( std::string type_name) : type_name(type_name) {}; //No default arg to ensure constructing correctly
        TypeName(const TypeName& other) : type_name(other.get()) {}
        TypeName& operator=(const TypeName& other) {
            if (this != &other) {
                type_name = other.get();
            }
            return *this;
        }
        bool operator==(const TypeName& other) const {
            try {
                //Checks for equality to Any
                if (type_name == "_" || other.get() == "_") { return true; } //Any equal to others
                if (type_name == other.get()) { return true; } //If equal then return true
                //Checks for null pointer equality to other pointers
                return type_name.at(0) == '&' && other.get().at(0) == '&' && (type_name.at(1) == '_' || other.get().at(1) == '_');
            } catch (const std::out_of_range& e) {} 
            return false;
        }
        bool operator!=(const TypeName& other) const {
            return !(*this == other);
        }
        std::string get() const { return type_name; }
};
struct Exp;
struct Type;
struct Function;

struct ParamsReturnVal {
    std::vector<Type*> params;
    Type* rettyp;
    ParamsReturnVal() {};
    ParamsReturnVal(const std::vector<Type*>& params, Type* rettyp) : params(params), rettyp(rettyp) {}
};

using Gamma = std::unordered_map<std::string, TypeName*>;
using Delta = std::unordered_map<std::string, Gamma>;
using Errors = std::map<std::string, std::vector<std::string>>;
using FunctionsInfo = std::unordered_map<std::string, ParamsReturnVal>;

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
        return TypeName(name);
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
            prms_type_names += prms[i]->typeName().get();
            if (i != prms.size() - 1) prms_type_names += ", ";
        }
        return TypeName("(" + prms_type_names + ") -> " + ret->typeName().get());
    }
    ParamsReturnVal funcInfo () const override { return ParamsReturnVal(prms, ret); }
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
                ptr_type_names += temp->typeName().get();
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
    bool isAny() const override { return true; }
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
    virtual std::string getName() { return nullptr; };
    virtual bool isAny() const { return false; }
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
};
struct ExpId : Exp {
    std::string name;
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() { return name; };
};
struct Nil : Exp {
    void print(std::ostream& os) const override { os << "Nil"; }
    TypeName typeName() const {
        return TypeName("&_");
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override { return TypeName("&_");};
};
struct UnOp : Exp {
    UnaryOp* op;
    Exp* operand;
    void print(std::ostream& os) const override { os << *op << "(" << *operand << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, operand);
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
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, left, right);
    };
    ~BinOp() { delete left; delete right; }
};
struct ExpArrayAccess : Exp {
    Exp* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~ExpArrayAccess() { delete ptr; delete index; }
};
struct ExpFieldAccess : Exp {
    Exp* ptr;
    std::string field;
    void print(std::ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
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
};
struct AnyExp : Exp {
    void print(std::ostream& os) const override { 
        os << "_";
    }
    TypeName typeName() const {
        return TypeName("_");
    }
    bool isAny() const override { return true; }
};

struct Lval {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual std::string getName() { return nullptr; };
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
    std::string getName() { return name; };
};
struct LvalDeref : Lval {
    Lval* lval;
    void print(std::ostream& os) const override { os << "Deref(" << *lval << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~LvalDeref() { delete lval; }
};
struct LvalArrayAccess : Lval {
    Lval* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~LvalArrayAccess() { delete ptr; delete index; }
};
struct LvalFieldAccess : Lval {
    Lval* ptr;
    std::string field;
    void print(std::ostream& os) const override {
        os << "FieldAccess(\nptr = " << *ptr << ",\nfield = " << field << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
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
    virtual bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const { return true; };
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
    bool typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const override;
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
    Type* type; //Fn type if this Decl is an extern
    std::vector<Decl*> params; //optional, used to keep track of parameters for extern decl
    ~Decl() { delete type; }
    friend std::ostream& operator<<(std::ostream& os, const Decl& decl) {
        os << "Decl(" << decl.name << ", " << *(decl.type) << ")";
        return os;
    }
    TypeName typeName() const { 
        return type->typeName();
    }
    ParamsReturnVal funcInfo () const { 
        return type->funcInfo();
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
    TypeName typeName() const {
        return TypeName(name);
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
            prms_type_names += params[i]->typeName().get();
            if (i != params.size() - 1) prms_type_names += ", ";
        }
        return TypeName("&(" + prms_type_names + ") -> " + rettyp->typeName().get()); //Functions should return a pointer to a function
    }
    ParamsReturnVal funcInfo () const { 
        std::vector<Type*> prms;
        for (Decl* decl: params) {
            prms.push_back(decl->type);
        }
        return ParamsReturnVal(prms, rettyp); 
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


//General functions (with _TC for type check) and others which depend on earlier definitions
ParamsReturnVal Type::funcInfo () const {
    return ParamsReturnVal(std::vector<Type*>(), new Any());
}

TypeName const id_TC(Gamma& gamma, const Function* fun, Errors& errors, std::string name) {
    try {
        TypeName* temp = gamma.at(name); //Returns TypeName* struct
        return *temp;
    } catch (const std::out_of_range& e) {
        errors["[ID]"].push_back("[ID] in function " + fun->name + ": variable " + name + " undefined");
        gamma[name] = new TypeName("_"); //this should fix the types for expression
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
    if (exp_type == TypeName("_")) { return exp_type; }
    if ( exp_type != TypeName("&_") && exp_type.get()[0] != '&') {
        errors["[NEG]"].push_back("[NEG] in function " + fun->name + ": dereferencing type " + exp_type.get() + " instead of pointer");
        return TypeName("_");
    }
    return TypeName( exp_type.get().substr(1) );
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
        errors["[NEG]"].push_back("[NEG] in function " + fun->name + ": negating type " + exp_type.get() + " instead of int");
    }
    return TypeName("int");
};

TypeName BinaryOp::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, fun, errors) );
    TypeName right_type ( right->typeCheck(gamma, fun, errors) );
    if ( left_type != TypeName("int") ) {
        errors["[BINOP-REST]"].push_back("[BINOP-REST] in function " + fun->name + ": operand has type " + left_type.get() + " instead of int");
    }
    if ( right_type != TypeName("int") ) {
        errors["[BINOP-REST]"].push_back("[BINOP-REST] in function " + fun->name + ": operand has type " + right_type.get() + " instead of int");
    }
    return TypeName("int");
}

TypeName Equal::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, fun, errors) );
    TypeName right_type ( right->typeCheck(gamma, fun, errors) );
    if (left_type != TypeName("int") && left_type.get()[0] != '&') {
        errors["[BINOP-EQ]"].push_back("[BINOP-EQ] in function " + fun->name + ": operand has non-primitive type " + left_type.get());
    }
    if (right_type != TypeName("int") && right_type.get()[0] != '&') {
        errors["[BINOP-EQ]"].push_back("[BINOP-EQ] in function " + fun->name + ": operand has non-primitive type " + right_type.get());
    }
    if (left_type != right_type) {
        errors["[BINOP-EQ]"].push_back("[BINOP-EQ] in function " + fun->name + ": operands with different types: " + left_type.get() + " vs " + right_type.get());
    }
    return TypeName("int");
}

TypeName const arrayAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> ptr, Exp* index) {
    TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    TypeName index_type ( index->typeCheck(gamma, fun, errors) );
    if (index_type != TypeName("int")) {
        errors["[BINOP-EQ]"].push_back("[BINOP-EQ] in function " + fun->name + ": array index is type " + index_type.get() + " instead of int");
    }
    if (ptr_type.get() != "_" && ptr_type.get()[0] != '&') { //don't have to worry about dereferencing nil
        errors["[BINOP-EQ]"].push_back("[BINOP-EQ] in function " + fun->name + ": dereferencing non-pointer type " + index_type.get());
        return TypeName("_");
    }
    return TypeName( ptr_type.get().substr(1) );
}
TypeName ExpArrayAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return arrayAccess_TC(gamma, fun, errors, ptr, index);
}
TypeName LvalArrayAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return arrayAccess_TC(gamma, fun, errors, ptr, index);
}

TypeName const fieldAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> ptr, std::string field) {
    extern Delta delta; //Cannot have a nil call, like nil(), that is a parse error
    TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    if (ptr_type.get() == "_") { return TypeName("_"); } //errors won't happpen given Any struct
    if ( ptr_type.get()[0] != '&' && ptr_type.get().substr(0,2) == "&(") { //if accessing something other than a struct type
        errors["[FIELD]"].push_back("[FIELD] in function " + fun->name + ": accessing field of incorrect type " + ptr_type.get());
        return TypeName("_");
    }
    if (delta.find(ptr_type.get()) == delta.end() ) {  // If the iterator points to the end of the map, the key doesn't exist
        errors["[FIELD]"].push_back("[FIELD] in function " + fun->name + ": accessing field of non-existent struct type " + ptr_type.get().substr(1));
        return TypeName("_"); 
    }
    if (delta[ptr_type.get()].find(field) == delta[ptr_type.get()].end()) {
        errors["[FIELD]"].push_back("[FIELD] in function " + fun->name + ": accessing non-existent field " + field + " of struct type " + ptr_type.get());
        return TypeName("_"); 
    }
    return *(delta[ptr_type.get()][field]);
}
TypeName ExpFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return fieldAccess_TC(gamma, fun, errors, ptr, field);
}
TypeName LvalFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return fieldAccess_TC(gamma, fun, errors, ptr, field);
}

std::pair<TypeName, bool> call_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> callee, std::vector<Exp*> arg) {
    extern FunctionsInfo functions_map; //Cannot have a nil call, like nil(), that is a parse error
    TypeName callee_type = std::visit([&callee_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, callee);
    std::string callee_name = std::visit([&callee_type](auto* arg) { return arg->getName(); }, callee);
    if (callee_type.get() == "_") { return std::pair<TypeName, bool>(TypeName("_"), false); } //callee is just any, just return undefined error which typeCheck already added
    bool success = true;
    // std::cout << "Before main" << std::endl;
    if (callee_type == TypeName("main")) { //If so, can't be an extern call
        errors["[ECALL-INTERNAL]"].push_back("[ECALL-INTERNAL] in function " + fun->name + ": calling main");
        success = false;
    }
    // std::cout << "In call_TC, callee name: " << callee_name << std::endl;
    ParamsReturnVal prv = functions_map[callee_name]; //can assume it exists since we already returned for values not in gamma
    // std::cout << "Return type of prv: " << prv.rettyp->typeName().get() << std::endl;
    std::string expression_statement = std::holds_alternative<Exp*>(callee) ? "[ECALL" : "[SCALL";
    std::string internal_external = ( callee_type.get()[0] == '&' ) ? "-INTERNAL]" : "-EXTERN]";
    std::string error_type = expression_statement + internal_external;
        std::cout << "Before empty return type" << std::endl;
    if (expression_statement != "[SCALL" && prv.rettyp->typeName().get() == "_") { //empty return type
        errors[error_type].push_back(error_type + " in function " + fun->name + ": calling a function with no return value");
        success = false;
    }
    // std::cout << "Before parameters number" << std::endl;
    if (prv.params.size() != arg.size()) {
        errors[error_type].push_back(error_type + " in function " + fun->name + 
        ": call number of arguments (" + std::to_string(arg.size()) + ") and parameters (" + std::to_string(prv.params.size()) + ") don't match");
        success = false;
    }
    // std::cout << "Before parameters types" << std::endl;
    unsigned int min = (prv.params.size() < arg.size()) ? prv.params.size() : arg.size();
    for (unsigned int i = 0; i < min; i++) {
        TypeName param_type = prv.params[i]->typeName();
        TypeName arg_type = arg[i]->typeCheck(gamma, fun, errors);
        if (param_type != arg_type) {
            errors[error_type].push_back(error_type + " in function " + fun->name + ": call argument has type " + arg_type.get() 
                + " but parameter has type " + param_type.get());
        }
        success = false;
    }
    return std::pair<TypeName, bool>(prv.rettyp->typeName(), success);
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
    TypeName exp_type ( exp->typeCheck(gamma, fun, errors) ); //store typename given, typeCheck will replace undefined variables with Any
    if ( exp_type.get() == "_") { return true; }
    if ( exp_type.get() != return_type.get()) { //If not equal, then at least one of the types not _ and they are different
        if ( is_return_type_any && fun->rettyp->typeName().get() == "_" && !is_return_exp_any) { //If it wasnt Any before
            errors["[RETURN-1]"].push_back("[RETURN-1] in function " + fun->name + ": should return nothing but returning " + exp_type.get());
        } else if ( return_type.get() != "_" && is_return_exp_any ) { //If it was Any before
            errors["[RETURN-2]"].push_back("[RETURN-2] in function " + fun->name + ": should return " + return_type.get() + " but returning nothing");
        } else {
            errors["[RETURN-2]"].push_back("[RETURN-2] in function " + fun->name + ": should return " + return_type.get() + " but returning " + exp_type.get());
        }
        return false;
    }
    return true;
};