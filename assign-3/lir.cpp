#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>

#include "ast.cpp"

namespace LIR {
    
    struct Type {
        virtual void print(std::ostream& os) const {}
        virtual TypeName typeName() const { return TypeName("_"); }
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
                prms_type_names += prms[i]->typeName().type_name;
                if (i != prms.size() - 1) prms_type_names += ", ";
            }
            std::string temp = "(" + prms_type_names + ") -> " + ret->typeName().type_name;
            return TypeName(temp);
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
    };
    struct Any : Type {
        void print(std::ostream& os) const override { 
            os << "_";
        }
        TypeName typeName() const override {
            return TypeName("_");
        }
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

    struct Function {
        std::string name;
        std::vector<Decl*> params;
        // WARNING: if function has no return type, rettyp is Any : Type
        Type* rettyp;
        // WARNING: for each local, if there is no value after declaration, Exp is AnyExp : Exp
        std::vector<std::pair<Decl*, Exp*>> locals;
        //std::vector<Stmt*> stmts;
        ~Function() {
            for (Decl* decl: params) delete decl;
            delete rettyp;
            for (auto [decl, exp]: locals) { delete decl; delete exp; }
            //for (Stmt* stmt: stmts) delete stmt;
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
            // for (unsigned int i = 0; i < func.stmts.size(); i++) {
            //     os << *(func.stmts[i]);
            //     if (i != func.stmts.size() - 1) os << ", ";
            // }
            os << "]\n)";
            return os;
        }
    };

    // Feel free to work on converting AST::Program to LIR::Program
    // DO NOT work on BasicBlock, LirInst, Terminal, Operand, ArithmeticOp, ComparisonOp
    // I have an idea... will explain later 
}