#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>

class TypeName {
    private:
        std::string type_name;
        std::string struct_name; //used to pass in for exp or *
        std::string field_name;  //used to pass in for exp or *
    public:
        TypeName() {};
        TypeName( std::string t, std::string s = "", std::string f = "") : type_name(t), struct_name(s), field_name(f) {}; //No default arg to ensure constructing correctly
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
                // // // std::cout << "\nChecking for null: " << (type_name.at(0) == '&' && other.get().at(0) == '&' && (type_name.at(1) == '_' || other.get().at(1) == '_')) << "\n";
                // // // std::cout << "Lhs type name is " << type_name << " Rhs type name is: " << other.get() << "\n";
                return type_name.at(0) == '&' && other.get().at(0) == '&' && (type_name.at(1) == '_' || other.get().at(1) == '_');
            } catch (const std::out_of_range& e) {} 
            return false;
        }
        bool operator!=(const TypeName& other) const {
            // // // std::cout << "\n\n\nLhs type name is " << type_name << " Rhs type name is: " << other.get() << "\n";
            // bool temp = !(*this == other);
            // // // std::cout << "Result of not equal: " << temp << "\n";
            // return temp;
            return !(*this == other);
        }
        std::string get() const { return type_name; }
        std::string getStructName() const { return struct_name; }
        std::string getFieldName() const { return field_name; }
};
struct Exp;
struct Type;
struct Function;
extern const bool isFunction(const std::string& type_name);
extern const bool isFunctionNotPointer(const std::string& type);
extern const bool isStruct(const std::string& type);
extern const bool isValidFieldAcesss(const std::string& type);

struct ParamsReturnVal {
    std::vector<Type*> params;
    Type* rettyp;
    ParamsReturnVal() {};
    ParamsReturnVal(const std::vector<Type*>& params, Type* rettyp) : params(params), rettyp(rettyp) {}
    ParamsReturnVal& operator=(const ParamsReturnVal& other) {
        if (this != &other) { // Avoid self-assignment
            params = other.params;
            rettyp = other.rettyp;
        }
        return *this;
    }
};

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
        std::string temp = "(" + prms_type_names + ") -> " + ret->typeName().get();
        // // // std::cout << "About to return from typeName() Fn with type " << temp << "\n";
        return TypeName(temp);
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
        // // // std::cout << "Pointer type\n";
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
        // if (ptr_type_names == "") {
        //     // // std::cout << "Error here: with reference of type " << ref->typeName().get() << "\n";
        // }
        return TypeName(ptr_type_names);
    }
    ParamsReturnVal funcInfo () const override { return ref->funcInfo(); }
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
    virtual std::string getName() { 
        // std::cout << "\n\nPrinting name in exp\n\n";
        return "_";
    };
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
    std::string getName() override {
        // std::cout << "\n\nPrinting name in num:exp\n\n";
        return "_"; 
    }
};
struct ExpId : Exp {
    std::string name;
    ExpId() {}
    ExpId(const std::string& name) : name(name) {}
    void print(std::ostream& os) const override { os << "Id(" << name << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { 
        // std::cout << "\n\nPrinting name in expid:exp\n\n";
        return name; 
    }
};
struct Nil : Exp {
    void print(std::ostream& os) const override { os << "Nil"; }
    TypeName typeName() const {
        return TypeName("&_");
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override { return TypeName("&_");};
    std::string getName() override {
        // std::cout << "\n\nPrinting name in nil:exp\n\n";
        return "_"; 
    }
};
struct UnOp : Exp {
    UnaryOp* op;
    Exp* operand;
    void print(std::ostream& os) const override { os << *op << "(" << *operand << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override {
        return op->typeCheck(gamma, fun, errors, operand);
    }
    std::string getName() override {
        // std::cout << "\n\nPrinting name in unop:exp\n\n";
        return operand->getName(); 
    }
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
    std::string getName() override {
        // std::cout << "\n\nPrinting name in binop:exp\n\n";
        return "_"; 
    }
    ~BinOp() { delete left; delete right; }
};
struct ExpArrayAccess : Exp {
    Exp* ptr;
    Exp* index;
    void print(std::ostream& os) const override {
        os << "ArrayAccess(\nptr = " << *ptr << ",\nindex = " << *index << "\n)";
    }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override {
        // std::cout << "\n\nPrinting name in exparrayaccess:exp\n\n";
        return ptr->getName(); 
    }
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
    std::string getName() override {
        // std::cout << "\n\nPrinting name in expfieldaccess:exp\n\n";
        std::string temp = field;
        // std::cout << "ExpFieldAccess giving name " << temp << "\n";
        return temp;
        // return ptr->getName(); 
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
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    ~ExpCall() {
        delete callee;
        for (Exp* exp: args) delete exp;
    }
    std::string getName() override {
        // std::cout << "\n\nPrinting name in expcall:exp\n\n";
        return callee->getName(); 
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
    std::string getName() override {
        // std::cout << "\n\nPrinting name in nil:exp\n\n";
        return "_"; 
    }
};

struct Lval {
    virtual void print(std::ostream& os) const {}
    virtual TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const { return TypeName("_"); };
    virtual std::string getName() { 
        // std::cout << "\n\nPrinting null name in lval\n\n";
        return "_"; 
    };
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
    std::string getName() override { return name; };
};
struct LvalDeref : Lval {
    Lval* lval;
    void print(std::ostream& os) const override { os << "Deref(" << *lval << ")"; }
    TypeName typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const override;
    std::string getName() override { 
        // std::cout << "\n\nPrinting name in exp\n\n";
        return lval->getName();
    };
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
    std::string getName() override { 
        // std::cout << "\n\nPrinting name in lvalfieldaccess\n\n" << std::endl;
        return ptr->getName(); 
    };
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
    virtual std::string getName() { 
        // std::cout << "\n\nPrinting name in exp\n\n";
        return "_";
    };
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
    std::string getName() override { 
        // std::cout << "\n\nPrinting name in exp\n\n";
        return callee->getName();
    };
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
    bool typeCheck(Gamma& gamma, Errors& errors) const;
};








//General functions (with _TC for type check) and others which depend on earlier definitions
ParamsReturnVal Type::funcInfo () const {
    return ParamsReturnVal(std::vector<Type*>(), new Any());
}

TypeName RhsExp::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return exp->typeCheck(gamma, fun, errors);
}

TypeName const id_TC(Gamma& gamma, const Function* fun, Errors& errors, std::string name) {
    try {
        TypeName* temp = gamma.at(name); //Returns TypeName* struct
        return *temp;
    } catch (const std::out_of_range& e) {
        // std::cout << "Correct undefined\n";
        errors.push_back("[ID] in function " + fun->name + ": variable " + name + " undefined");
        // gamma[name] = new TypeName("_"); //this should fix the types for expression
        return TypeName("_");
    }
}
TypeName ExpId::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    // std::cout << "Expid\n";
    return id_TC(gamma, fun, errors, name);
};
TypeName LvalId::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    // std::cout << "Lvalid\n";
    return id_TC(gamma, fun, errors, name);
};

TypeName const deref_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> operand) {
    TypeName exp_type = std::visit([&exp_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, operand);
    if (exp_type.get() == "_") { return exp_type; }
    if ( exp_type != TypeName("&_") && exp_type.get()[0] != '&') {
        errors.push_back("[DEREF] in function " + fun->name + ": dereferencing type " + exp_type.get() + " instead of pointer");
        return TypeName("_");
    }
    return TypeName( exp_type.get().substr(1) );
}
TypeName UnaryDeref::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const {
    // std::cout << "IN UNARY DEREF\n\n";
    return deref_TC(gamma, fun, errors, operand);
}
TypeName LvalDeref::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return deref_TC(gamma, fun, errors, lval);
}

TypeName Neg::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* operand) const {
    TypeName exp_type ( operand->typeCheck(gamma, fun, errors) );
    if (exp_type != TypeName("int")) {
        errors.push_back("[NEG] in function " + fun->name + ": negating type " + exp_type.get() + " instead of int");
    }
    return TypeName("int");
};

TypeName BinaryOp::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, fun, errors) );
    TypeName right_type ( right->typeCheck(gamma, fun, errors) );
    if ( left_type != TypeName("int") ) {
        errors.push_back("[BINOP-REST] in function " + fun->name + ": operand has type " + left_type.get() + " instead of int");
    }
    if ( right_type != TypeName("int") ) {
        errors.push_back("[BINOP-REST] in function " + fun->name + ": operand has type " + right_type.get() + " instead of int");
    }
    return TypeName("int");
}

TypeName Equal::typeCheck(Gamma& gamma, const Function* fun, Errors& errors, Exp* left, Exp* right) const {
    TypeName left_type ( left->typeCheck(gamma, fun, errors) );
    TypeName right_type ( right->typeCheck(gamma, fun, errors) );
    if (left_type != TypeName("int") && left_type.get()[0] != '&') {
        errors.push_back("[BINOP-EQ] in function " + fun->name + ": operand has non-primitive type " + left_type.get());
    }
    if (right_type != TypeName("int") && right_type.get()[0] != '&') {
        errors.push_back("[BINOP-EQ] in function " + fun->name + ": operand has non-primitive type " + right_type.get());
    }
    if (left_type != right_type) {
        errors.push_back("[BINOP-EQ] in function " + fun->name + ": operands with different types: " + left_type.get() + " vs " + right_type.get());
    }
    return TypeName("int");
}

TypeName const arrayAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> ptr, Exp* index) {
    TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    TypeName index_type = index->typeCheck(gamma, fun, errors);
    // // // std::cout << "In arrayAccess_TC, index_type: " << index_type.get() << " and ptr type: " << ptr_type.get() << "\n";
    if (ptr_type.get() == "_") { return TypeName("_"); }
    if (index_type != TypeName("int")) {
        // // std::cout << "Error is for ptr type: " << ptr_type.get() << " with index type " << index_type.get() << "\n";
        errors.push_back("[ARRAY] in function " + fun->name + ": array index is type " + index_type.get() + " instead of int");
    }
    if (ptr_type.get() != "_" && ptr_type.get()[0] != '&') { //don't have to worry about dereferencing nil
        errors.push_back("[ARRAY] in function " + fun->name + ": dereferencing non-pointer type " + ptr_type.get());
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

TypeName const fieldAccess_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> ptr, std::string field, bool isCall=false) {
    if (isCall) { return std::visit([&gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);}

    // std::cout << "IN FIELD ACCESS\n";
    extern Delta delta; //Cannot have a nil call, like nil(), that is a parse error
    // std::cout << "Before typecheck\n";
    TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, ptr);
    // std::cout << "After typecheck\n";
    if (ptr_type.get() == "_") { return TypeName("_"); } //errors won't happpen given Any struct
    // std::cout << "Past the _, ptr_type is " << ptr_type.get() << "\n";
    
    // ptr_type.get()[0] != '&' || ptr_type.get().find("&(") != std::string::npos || ptr_type.get().find("&int") != std::string::npos || ptr_type.get().substr(0,2) == "&&"
    // std::cout << "Is call? " << isCall << " and is valid field? " << !isValidFieldAcesss(ptr_type.get()) << "\n";
    std::string struct_type = ptr_type.get().substr(1);
    
    if (isCall) {
        // std::cout << "HERE: Returning struct type " << struct_type << " and field type " << field << "\n";
        return TypeName("_", struct_type, field);
    }
    
    if ( !isValidFieldAcesss(ptr_type.get())) { //if accessing something other than a struct type
        errors.push_back("[FIELD] in function " + fun->name + ": accessing field of incorrect type " + ptr_type.get());
        // std::cout << "Return 1\n";
        return TypeName("_"); //all three errors are mutually exclusive
    }
    // std::string struct_type = ptr_type.get().substr(1);
    // std::cout << "Trying to find struct_type " << struct_type << "\n";
    //Could be non-struct type if a call! Allowed to do that.
    if (delta.find(struct_type) == delta.end() ) {  // If the iterator points to the end of the map, the key doesn't exist
        // std::cout << "Could not find struct " << struct_type << "\n";
        errors.push_back("[FIELD] in function " + fun->name + ": accessing field of non-existent struct type " + struct_type);
        // std::cout << "Return 2\n";
        return TypeName("_");
    }
    // std::cout << "In struct " << struct_type << " trying to find field " << field << "\n";
    if (delta[struct_type].find(field) == delta[struct_type].end()) {
        // std::cout << "Here\n";
        errors.push_back("[FIELD] in function " + fun->name + ": accessing non-existent field " + field + " of struct type " + struct_type);
        // std::cout << "Return 3\n";
        return TypeName("_");
    }
    // std::cout << "Past, returning type " << (*(delta[struct_type][field])).get() << " and struct name " << struct_type << "\n";
    return TypeName((*(delta[struct_type][field])).get(), struct_type, field);
}
TypeName ExpFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName temp = fieldAccess_TC(gamma, fun, errors, ptr, field);
    // std::cout << "In expFieldAccess with type " << temp.get() << ", struct name " << temp.getStructName() << " and field " <<  temp.getFieldName() << "\n";
    return temp;
    // return fieldAccess_TC(gamma, fun, errors, ptr, field);
}
TypeName LvalFieldAccess::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
     TypeName temp = fieldAccess_TC(gamma, fun, errors, ptr, field);
    // std::cout << "In lvalFieldAccess with type " << temp.get() << ", struct name " << temp.getStructName() << " and field " <<  temp.getFieldName() << "\n";
    return temp;
    // return fieldAccess_TC(gamma, fun, errors, ptr, field);
}

std::pair<TypeName, bool> call_TC(Gamma& gamma, const Function* fun, Errors& errors, std::variant<Exp*, Lval*> callee, std::vector<Exp*> arg) {
    extern FunctionsInfo functions_map; //Cannot have a nil call, like nil(), that is a parse error
    extern StructFunctionsInfo struct_functions_map;
    // std::cout << "In call_TC\n\n";
    TypeName callee_type = std::visit([&callee_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, callee);
    bool isFieldAccess = std::visit([](auto* arg) { return arg->isFieldAccess(); }, callee);
    std::string callee_name = std::visit([](auto* arg) { return arg->getName(); }, callee); //field name for field, function for others
    if (callee_name == "_") { return std::pair<TypeName, bool>(TypeName("_"), true); } //no need to continue if Any

    // std::string callee_name = "";
    // std::cout << "Done with typing\n";
    // if (isFieldAccess) {
    //     // std::cout << "HI\n\n\nType name " << callee_type.get() << " Struct type " << callee_type.getStructName() << " and field type " << callee_type.getFieldName() << "\n";
    // }
    
    // // std::cout << "<Starting call_TC> for name " << callee_name << "\n";
    // // std::cout << "<Continuing call_TC> with callee_type " << callee_type.get() << "\n";
    // // std::cout << "After callee construction for callee name: " << callee_name << " and callee type: " << callee_type.get() << "\n";
    bool success = true;
    // // std::cout << "Before main" << std::endl;
    // if (callee_type.get() == "_") { return std::pair<TypeName, bool>(TypeName("_"), false); }
    std::string expression_statement = std::holds_alternative<Exp*>(callee) ? "[ECALL" : "[SCALL";
    if (callee_name == "main") { //If so, can't be an extern call
        // // // std::cout << expression_statement+"-INTERNAL]" << "\n\n";
        // // // std::cout << "Function name: " << fun->name << "\n";
        errors.push_back(expression_statement+"-INTERNAL] in function " + fun->name + ": calling main");
        success = false;
        // // // std::cout << "Past error pushing, main is: " << (gamma.find("main") == gamma.end()) << "\n";
    }
    // std::cout << "In function " << fun->name << " callee type is " << callee_type.get() << "\n";
    if (callee_type.get() == "_") { return std::pair<TypeName, bool>(TypeName("_"), true); }
    // std::cout << "Checking if function type: " << callee_type.get() << "\n";
    if (!isFunction(callee_type.get())) { //main can be a parameter, so should check if bad type
        errors.push_back(expression_statement+"-*] in function " + fun->name + ": calling non-function type " + callee_type.get());
        return std::pair<TypeName, bool>(TypeName("_"), false);
    }
    if (callee_name == "main" && functions_map.find("main") == functions_map.end()) { //if main not defined as a function, return
            return std::pair<TypeName, bool>(TypeName("_"), false);
    }
    if (callee_type.get() == "_") { return std::pair<TypeName, bool>(TypeName("_"), true); } //callee is just any, just return undefined error which typeCheck already added (and possibly main error)
    // std::cout << "In call_TC, callee name: " << callee_name << std::endl;
    
    
    
    
    // std::cout << "Callee name " << callee_name << " with Callee type " << callee_type.get() << "is expaccess? " << isFieldAccess << "\n";
    // std::cout << "After isFieldAccess\n";
    ParamsReturnVal prv;
    
    
    if ( isFieldAccess ) { //can assume struct name exists, not field
        // std::cout << "Here with callee\n";
        // // std::cout << "<START>\n";
        // for ( Exp* exp: arg) { 
        //     // std::cout << exp->typeCheck(gamma, fun, errors).get(); 
        // }
        // // std::cout << "<END>\n";
        
        
        // TypeName ptr_type = std::visit([&ptr_type, &gamma, &fun, &errors](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, callee);
        // std::string struct_name = ptr_type.get().substr(1);
        // ExpId* struct_exp =  new ExpId(callee_name);
        // // std::cout << "BEFORE\n\n";
        // TypeName field_type;
        // if (!isField) {
        // TypeName type = std::visit([](auto* arg) { return arg->typeCheck(gamma, fun, errors); }, callee);
        // TypeName type = fieldAccess_TC(gamma, fun, errors, callee, callee_name, true);
        // std::cout << "Right after attempt\n\n";
        // } else {
        //     field_type = callee_name;
        // }
        // TypeName type = fieldAccess_TC(gamma, fun, errors, new ExpId(callee_name),"hi", true); //propogates errors
        // TypeName type = ;
        std::string struct_name = callee_type.getStructName();
        std::string field_name = callee_type.getFieldName();
        // // std::cout << "AFTER\n\n";
        // // std::cout << "Success, struct type " << struct_name << " field type " << field_name << "\n";
        // std::cout << "About to call typeCheck on type " << struct_name << "\n\n\n";
        // std::string struct_type = struct_exp->typeCheck(gamma, fun, errors).get().substr(1);
        prv = struct_functions_map[struct_name][field_name];
    } else { //can assume it exists since we already returned for values not in gamma
        // std::cout << "Here2 with callee_name " << callee_name << "\n";
        prv = functions_map[callee_name]; 
    }
    
    // std::cout << "Return type of prv: " << prv.rettyp->typeName().get() << std::endl;
    // // std::cout << "Number of parameters: " << prv.params.size() << "\n";
    std::string internal_external = ( callee_type.get()[0] == '&' ) ? "-INTERNAL]" : "-EXTERN]";
    std::string error_type = expression_statement + internal_external;
    // // std::cout << "Before empty return type" << std::endl;
    if (expression_statement != "[SCALL" && prv.rettyp->typeName().get() == "_") { //empty return type
        errors.push_back(error_type + " in function " + fun->name + ": calling a function with no return value");
        success = false;
    }
    // std::cout << "Before parameters number" << std::endl;
    if (prv.params.size() != arg.size()) {
        errors.push_back(error_type + " in function " + fun->name + 
        ": call number of arguments (" + std::to_string(arg.size()) + ") and parameters (" + std::to_string(prv.params.size()) + ") don't match");
        success = false;
    }
    // std::cout << "Before parameters types" << std::endl;
    unsigned int min = (prv.params.size() < arg.size()) ? prv.params.size() : arg.size();
    for (unsigned int i = 0; i < min; i++) {
        TypeName param_type = prv.params[i]->typeName();
        TypeName arg_type = arg[i]->typeCheck(gamma, fun, errors);
        if (param_type != arg_type) {
            errors.push_back(error_type + " in function " + fun->name + ": call argument has type " + arg_type.get() 
                + " but parameter has type " + param_type.get());
            success = false;
        }
    }
    // std::cout << "Done with function\n";
    // std::cout << "Returning type " << prv.rettyp->typeName().get() << "\n";
    std::pair<TypeName, bool> temp(prv.rettyp->typeName(), success);
    // std::cout << "Seg after here\n";
    return std::pair<TypeName, bool>(prv.rettyp->typeName(), success);
}

TypeName ExpCall::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    return call_TC(gamma, fun, errors, callee, args).first;
}

bool StmtCall::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    // std::cout << "Statement about to enter...\n";
    return call_TC(gamma, fun, errors, callee, args).second;
}

bool Return::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    bool is_return_type_any = fun->rettyp->isAny();
    bool is_return_exp_any = exp->isAny();
    TypeName return_type = fun->rettyp->typeName();
    TypeName exp_type = exp->typeCheck(gamma, fun, errors); //store typename given, typeCheck will replace undefined variables with Any
    // // // std::cout << "exp type " << exp_type.get() << " is any? " << is_return_exp_any << "\n";
    // // // std::cout << "For function " << fun->name << "\nReturn type any: " << is_return_type_any << " Return exp any: " << is_return_exp_any << " Return type: " << return_type.get() << " Exp Type: " << exp_type.get() << "\n";
    if ( exp_type.get() == "_" && !is_return_exp_any) { return true; } //If exp was made to be Any in typeCheck
    if ( exp_type.get() != return_type.get()) { //If not equal, then at least one of the types not _ and they are different
        // // // std::cout << "Nil comparison for return type " << return_type.get() << " and exp type "  << exp_type.get() << " " << (return_type.get() != "_" && exp_type != return_type) << "\n";
        // // // std::cout << "is_return_type_any " << is_return_type_any << "!is_return_exp_any" << !is_return_exp_any <<  "fun->rettyp->typeName().get() == \"_\"" << (fun->rettyp->typeName().get() == "_") << "\n";
        if ( is_return_type_any && !is_return_exp_any ) { //If it wasnt Any before
            errors.push_back("[RETURN-1] in function " + fun->name + ": should return nothing but returning " + exp_type.get());
        } else if ( return_type.get() != "_" && exp_type.get() == "_") { //If it was Any before
            errors.push_back("[RETURN-2] in function " + fun->name + ": should return " + return_type.get() + " but returning nothing");
        } else if (exp_type != return_type) {
            errors.push_back("[RETURN-2] in function " + fun->name + ": should return " + return_type.get() + " but returning " + exp_type.get());
        }
        return false;
    }
    return true;
};

TypeName New::typeCheck(Gamma& gamma, const Function* fun, Errors& errors) const {
    TypeName amount_type = amount->typeCheck(gamma, fun, errors);
    TypeName type_typename = type->typeName();
    if((amount_type.get() != "int") && (amount_type.get() != "_")) {
        errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": allocation amount is type " + amount_type.get() + " instead of int");
    }
    if(isFunctionNotPointer(type_typename.get())) {
        errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": allocating function type " + type_typename.get());
    }
    return TypeName("new " + type_typename.get());
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
    if((rhs_type.get() == "new _") || lhs_type == rhs_type){
        // std::cout << "First lhs type " << lhs_type.get() << " rhs type " << rhs_type.get() << "\n";   
        return true;
    } else {
        if(rhs_type.get().substr(0,3) != "new") {
            // std::cout << "Here lhs type " << lhs_type.get() << " rhs type " << rhs_type.get() << "\n";      
            if((lhs_type.get() != rhs_type.get())){ //already not any from top if statement
                errors.push_back("[ASSIGN-EXP] in function " + fun->name + ": assignment lhs has type " + lhs_type.get() + " but rhs has type " + rhs_type.get());
                tf = false;
            }
            if(isFunctionNotPointer(lhs_type.get()) || isStruct(lhs_type.get())) {
                errors.push_back("[ASSIGN-EXP] in function " + fun->name + ": assignment to struct or function");
                tf = false;
            }
        } 
        else if(rhs_type.get().substr(4) != lhs_type.get()){
            if((lhs_type.get()[0] == '&') && (rhs_type.get().substr(4)[0] == '&')){
                return true;
            }
            else{
                // std::cout << "Lhs/Rhs lhs type " << lhs_type.get().substr(1) << " rhs type " << rhs_type.get() << "\n";      
                if((lhs_type.get()[0] == '&') && (rhs_type.get().substr(4,5) == "int" || lhs_type.get().substr(1) == rhs_type.get().substr(4))){
                    return true;
                }
                errors.push_back("[ASSIGN-NEW] in function " + fun->name + ": assignment lhs has type " + lhs_type.get() + " but we're allocating type " + rhs_type.get().substr(4));
                tf = false;
            }     
        }
    }
    return tf;
}

bool If::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    TypeName exp_type (guard->typeCheck(gamma, fun, errors));
    bool tf = true;
    if((exp_type.get() != "int") && (exp_type.get() != "_")) {
        errors.push_back("[IF] in function " + fun->name + ": if guard has type " + exp_type.get() + " instead of int");
        tf = false;
    }
    for(auto s: tt){ s->typeCheck(gamma, fun, loop, errors); }
    for(auto s: ff){ s->typeCheck(gamma, fun, loop, errors); }
    return tf;
}
bool While::typeCheck(Gamma& gamma, const Function* fun, bool loop, Errors& errors) const {
    TypeName exp_type (guard->typeCheck(gamma, fun, errors));
    if((exp_type.get() != "int") && (exp_type.get() != "_")) {
        errors.push_back("[WHILE] in function " + fun->name + ": while guard has type " + exp_type.get() + " instead of int");
    }
    for(auto s: body){ s->typeCheck(gamma, fun, true, errors); } 
    return true;
}

bool global_TC(Gamma& gamma, Errors& errors, std::string global_name, Type* type) {
    bool tf = true;
    TypeName global_type = type->typeName();
    if ( isStruct(global_type.get()) || isFunctionNotPointer(global_type.get()) ) { 
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
        if ( isStruct(field_type.get()) || isFunctionNotPointer(field_type.get()) ) { 
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
        if ( isStruct(var_type.get()) || isFunctionNotPointer(var_type.get()) ) { 
            // // // std::cout << "[FUNCTION] in function " + name + ": variable " + decl->name + " has a struct or function type\n";
            errors.push_back("[FUNCTION] in function " + name + ": variable " + decl->name + " has a struct or function type");
            tf = false; 
        }
    }
    for (auto [decl, exp]: locals) {
        TypeName var_type = decl->typeName();
        // // // std::cout << "Testing var " << decl->name << " with type " << var_type.get() << "\n";
        // // // std::cout << "[FUNCTION] in function " + name + ": variable " + decl->name + " has a struct or function type\n";
        if ( isStruct(var_type.get()) || isFunctionNotPointer(var_type.get()) ) { 
            // // // std::cout << "[FUNCTION] in function " + name + ": variable " + decl->name+ " has a struct or function type\n";
            errors.push_back("[FUNCTION] in function " + name + ": variable " + decl->name+ " has a struct or function type");
            tf = false; 
        }
        TypeName exp_type = exp->typeCheck(gamma, fun, errors);
        if (var_type != exp_type) {
            errors.push_back("[FUNCTION] in function " + name + ": variable " + decl->name + " with type " + var_type.get() + " has initializer of type " + exp_type.get());
            tf = false; 
        }
    }
    for (Stmt* stmt : stmts) {
        if (!stmt->typeCheck(gamma, fun, false, errors)) { tf = false; }
    }
    return tf;
}

bool Program::typeCheck(Gamma& gamma, Errors& errors) const { //input gamma is the initial gamma
    extern std::unordered_map<std::string, Gamma> locals_map;
    bool tf = true;
    for (Decl* decl: globals) { 
        if (!global_TC(gamma, errors, decl->name, decl->type)) { tf = false; }
    }
    for (Struct* str: structs) {
        if (!str->typeCheck(gamma, errors)) { tf = false; }
    }
    for (Function* fun: functions) { //Creating locals map
        // // // std::cout << "Checking function " << fun->name << std::endl;
        if (!fun->typeCheck(locals_map[fun->name], fun, errors)) { tf = false; }
    } 
    return tf;
}