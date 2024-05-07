#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <map>
#include <variant>
#include <tuple>

#include "ast.cpp"

using namespace AST;

namespace LIR {

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

        // ADD LOGIC FOR LIR FUNCTION BODY
        // std::vector<Stmt*> stmts;


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

            // PRINT FOR LIR FUNCTION BODY

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