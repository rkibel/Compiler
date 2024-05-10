#include <typeinfo>

#include "ast.cpp"
using namespace std;

namespace LIR {

    // these are declared in main.cpp
    extern Gamma globals_map; // stores globals, externs, and functions
    extern Delta delta; // struct name to (struct decl to type)
    extern unordered_map<string, Gamma> locals_map; // function name to its locals (params and locals)
    extern unordered_map<string, ParamsReturnVal> functions_map; // all functions to their funcInfo
    extern unordered_map<string, FunctionsInfo> struct_functions_map; // struct name to (struct decl to funcInfo)
    
    struct Function {
        AST::Function* func;
        vector<string> tempsToType; // stores which type each temp has, ie if _t2 is type Int, Int will be in tempsToType[1]
        unsigned int numLabels = 0;
        Function(AST::Function* func, map<string, string> globals_map, map<string, map<string, string>> struct_map, map<string, string> extern_map) : func(func) {}
        
        string lowerFunc() {
            // loweredLocalExpStmts takes a local declaration, ie let x: int = 3;
            // then creates an AST::Assign with Lval x and Rhs 3
            // Then lowers this Stmt and stores the resulting instructions
            vector<string> loweredLocalExpStmts;
            map<string, string> localsMap;
            for (auto& pair: func->locals) {
                localsMap[pair.first->name] = pair.first->type->toLIRType();
                if (typeid(*(pair.second)) == typeid(AST::AnyExp)) { continue; }
                else {
                    AST::RhsExp* rhs = new RhsExp();
                    rhs->exp = pair.second;
                    AST::LvalId* lvalid = new LvalId();
                    lvalid->name = pair.first->name;
                    AST::Assign* assign = new Assign();
                    assign->lhs = lvalid;
                    assign->rhs = rhs;
                    loweredLocalExpStmts.push_back(lowerStmt(assign));
                }
            }
            // stores lowering of each of func's statements
            vector<string> loweredStmts;
            for (AST::Stmt* stmt: func->stmts) loweredStmts.push_back(lowerStmt(stmt));
            
            string res = "Function " + func->name + "(";
            for (unsigned int i = 0; i < func->params.size(); i++) {
                res +=  func->params[i]->name + ":" + func->params[i]->type->toLIRType();
                if (i != func->params.size() - 1) res += ", ";
            }
            res += ") -> " + func->rettyp->toLIRType() + " {\n  Locals\n";
            for (unsigned int i = 0; i < tempsToType.size(); i++) {
                res += "    _t" + to_string(i+1) + " : " + tempsToType[i] + "\n";
            }
            for (auto& pair: localsMap) {
                res += "    " + pair.first + " : " + pair.second + "\n";
            }
            res += "\n  entry:\n";
            for (string str: loweredLocalExpStmts) res += str;
            for (string str: loweredStmts) res += str;
            res += "}";
            return res;
        }
        
        // TODO
        string lowerStmt(AST::Stmt* stmt) {
            return "";
        }

        // the idea for lowering Exp is to evaluate the instructions of the operand first
        // whatever the operand is will be stored in a fresh type in pair.first
        // the instructions for evaluating this operand are in pair.second
        // then evaluate the current Exp
        // note: if no fresh type is needed, returns pair<0, ...>
        pair<unsigned int, string> lowerExp(AST::Exp* exp) {
            vector<string> errors_vec;
            return exp->lower(tempsToType, numLabels, globals_map, func, errors_vec);
            // TODO other lower methods in AST::Exp
        }

        // TODO
        string lowerLval(AST::Lval* lval) {
            return "";
        }

        // TODO
        string lowerLvalAsExp(AST::Lval* lval) {
            return "";
        }

    };

    struct Program {
        map<string, string> globals_map;
        map<string, map<string, string>> struct_map;
        map<string, string> extern_map;
        map<string, string> function_map;

        Program(AST::Program* prog) {
            for (AST::Decl* decl: prog->globals) { 
                globals_map[decl->name] = decl->type->toLIRType();
            }
            for (AST::Function* func: prog->functions) {
                if (func->name != "main") globals_map[func->name] = "Ptr(" + func->toLIRType() + ")";
            }
            for (AST::Struct* str: prog->structs) {
                map<string, string> temp_map;
                for (AST::Decl* decl: str->fields) {
                    temp_map[decl->name] = decl->type->toLIRType();
                }
                struct_map[str->name] = temp_map;
            }
            for (AST::Decl* decl: prog->externs) {
                extern_map[decl->name] = decl->type->toLIRType();
            }
            for (AST::Function* func: prog->functions) {
                Function helper(func, globals_map, struct_map, extern_map);
                function_map[func->name] = helper.lowerFunc();
            }
        }
        void print() {
            for (const auto& pair: struct_map) {
                cout << "Struct " << pair.first << "\n";
                for (const auto& pair1: pair.second) {
                    cout << "  " << pair1.first << " : " << pair1.second << "\n";
                }
                cout << "\n";
            }
            cout << "Externs\n";
            for (const auto& pair: extern_map) {
                cout << "  " << pair.first << " : " << pair.second << "\n";
            }
            cout << "\nGlobals\n";
            for (const auto& pair: globals_map) {
                cout << "  " << pair.first << " : " << pair.second << "\n";
            }
            cout << "\n";
            for (const auto& pair: function_map) {
                cout << pair.second << "\n\n";
            }
        }
    };

    // Feel free to work on converting AST::Program to LIR::Program
    // DO NOT work on BasicBlock, LirInst, Terminal, Operand, ArithmeticOp, ComparisonOp
}