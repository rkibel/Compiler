#include <typeinfo>
#include <sstream>
#include <iostream>

#include "ast.cpp"
using namespace std;

namespace LIR {

    // // these are declared in main.cpp
    extern Gamma globals_map; // stores globals, externs, and functions. Used unless you call another function, in which case you do locals_map[functions_name]
    extern Delta delta; // struct name to (struct decl to type)
    // extern unordered_map<string, Gamma> locals_map; // function name to its locals (params and locals) with global included
    extern unordered_map<string, ParamsReturnVal> functions_map; // all functions to their funcInfo
    extern unordered_map<string, FunctionsInfo> struct_functions_map; // struct name to (struct decl to funcInfo)
    
    struct Function {
        AST::Function* func;
        vector<string> tempsToType; // stores which type each temp has, ie if _t2 is type Int, Int will be in tempsToType[1]
        unsigned int numLabels = 0;
        unsigned int prevWhileHdr = 0;
        unsigned int prevWhileEnd = 0;
        map<string, string> extern_map;
        map<string, string> function_map;
        unordered_map<string, Type*> gamma;
        Function(AST::Function* func, map<string, string> extern_map, map<string, string> function_map, Gamma& gamma) : func(func), extern_map(extern_map), function_map(function_map), gamma(gamma) {}
        
        string lowerFunc() {
            // loweredStmts takes a local declaration, ie let x: int = 3;
            // then creates an AST::Assign with Lval x and Rhs 3
            // then lowers this Stmt and stores the resulting instructions
            // also stores following functions stmts to lowered form
            map<string, string> localsMap;
            vector<string> loweredStmts;
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
                    loweredStmts.push_back(assign->lower(tempsToType, numLabels, gamma, func, prevWhileHdr, prevWhileEnd, extern_map, function_map));
                }
            }
            for (AST::Stmt* stmt: func->stmts) loweredStmts.push_back(stmt->lower(tempsToType, numLabels, gamma, func, prevWhileHdr, prevWhileEnd, extern_map, function_map));
            
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
            for (string str: loweredStmts) res += str;
            res += "}";

            // this is the CFG construction
            istringstream iss(res);
            bool terminalBefore = false;
            string actual_res = "";
            string line;
            while(getline(iss, line)) {
                actual_res += "\n";
                if (line.substr(0, 5) == "  lbl" || line.substr(0, 1) == "}") terminalBefore = false;
                if (terminalBefore) continue;
                actual_res += line;
                if (line.substr(0, 8) == "    Jump" || line.substr(0, 7) == "    Ret") terminalBefore = true;
            }
            return actual_res;
        }
    };

    struct Program {
        map<string, string> global_map;
        map<string, map<string, string>> struct_map;
        map<string, string> extern_map;
        map<string, string> function_map;
        unordered_map<string, Gamma> locals_map;

        Program(AST::Program* prog, unordered_map<string, Gamma>& locals_map) : locals_map(locals_map) {
            for (AST::Decl* decl: prog->globals) { 
                global_map[decl->name] = decl->type->toLIRType();
            }
            for (AST::Function* func: prog->functions) {
                if (func->name != "main") global_map[func->name] = "Ptr(" + func->toLIRType() + ")";
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
                Function helper(func, extern_map, function_map, locals_map[func->name]);
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
            for (const auto& pair: global_map) {
                cout << "  " << pair.first << " : " << pair.second << "\n";
            }
            cout << "\n";
            for (const auto& pair: function_map) {
                cout << pair.second << "\n\n"; // second contains the full string of the function lowering
            }
        }
    };
}