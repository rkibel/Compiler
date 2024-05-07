#include "ast.cpp"

// using ParamsReturnVal = std::pair<std::vector<Type*>, Type*>;
// using Gamma = std::unordered_map<std::string, TypeName*>;
// using Delta = std::unordered_map<std::string, Gamma>;
// using Errors = std::vector<std::string>;
// using FunctionsInfo = std::unordered_map<std::string, ParamsReturnVal>;
// using StructFunctionsInfo = std::unordered_map<std::string, FunctionsInfo>;

namespace LIR {

    struct Program {
        std::map<std::string, std::string> globals_map;
        std::map<std::string, std::map<std::string, std::string>> struct_map;
        std::map<std::string, std::string> extern_map;

        Program(AST::Program* prog) {
            for (AST::Decl* decl: prog->globals) { 
                globals_map[decl->name] = decl->type->toLIRString();
            }
            for (AST::Function* func: prog->functions) {
                if (func->name != "main") globals_map[func->name] = "Ptr(" + func->toLIRString() + ")";
            }
            for (AST::Struct* str: prog->structs) {
                std::map<std::string, std::string> temp_map;
                for (AST::Decl* decl: str->fields) {
                    temp_map[decl->name] = decl->type->toLIRString();
                }
                struct_map[str->name] = temp_map;
            }
            for (AST::Decl* decl: prog->externs) {
                extern_map[decl->name] = decl->type->toLIRString();
            }
        }
        void print() {
            for (const auto& pair: struct_map) {
                std::cout << "Struct " << pair.first << "\n";
                for (const auto& pair1: pair.second) {
                    std::cout << "  " << pair1.first << " : " << pair1.second << "\n";
                }
                std::cout << "\n";
            }
            std::cout << "Externs\n";
            for (const auto& pair: extern_map) {
                std::cout << "  " << pair.first << " : " << pair.second << "\n";
            }
            std::cout << "\nGlobals\n";
            for (const auto& pair: globals_map) {
                std::cout << "  " << pair.first << " : " << pair.second << "\n";
            }
            std::cout << "\n";

        }
    };

    struct Function {
        std::vector<std::string> tempsToType; // this stores which type each temp has, ie if _t2 is type Int, Int will be in tempsToType[1]
        

    };

    // Feel free to work on converting AST::Program to LIR::Program
    // DO NOT work on BasicBlock, LirInst, Terminal, Operand, ArithmeticOp, ComparisonOp
}