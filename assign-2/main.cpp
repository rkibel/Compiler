#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "grammar.cpp"

using Gamma = std::unordered_map<std::string, TypeName*>;
using Delta = std::unordered_map<std::string, Gamma>;
using Errors = std::map<std::string, std::vector<std::string>>;
std::vector<std::string> tokens;
//Declare type maps for prog
std::unordered_map<std::string, TypeName*> globals_map; //used for creating local maps and for main function
std::unordered_map<std::string, std::unordered_map<std::string, TypeName*>> delta;
std::unordered_map<std::string, std::unordered_map<std::string, TypeName*>> locals_map; //keys for gamma are (non-main) function names, whose value is that function's gamma

std::map<std::string, std::vector<std::string>> errors_map = {
        {"[BINOP-REST]", {}},
        {"[BINOP-EQ]", {}},
        {"[ID]", {}},
        {"[NEG]", {}},
        {"[DEREF]", {}},
        {"[ARRAY]", {}},
        {"[FIELD]", {}},
        {"[ECALL-INTERNAL]", {}},
        {"[ECALL-EXTERN]", {}},
        {"[ECALL-*]", {}},
        {"[BREAK]", {}},
        {"[CONTINUE]", {}},
        {"[RETURN-1]", {}},
        {"[RETURN-2]", {}},
        {"[ASSIGN-EXP]", {}},
        {"[ASSIGN-NEW]", {}},
        {"[SCALL-INTERNAL]", {}},
        {"[SCALL-EXTERN]", {}},
        {"[SCALL-*]", {}},
        {"[IF]", {}},
        {"[WHILE]", {}},
        {"[GLOBAL]", {}},
        {"[STRUCT]", {}},
        {"[FUNCTION]", {}}
    };

bool isWhitespace(unsigned char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "No argument provided\n"; 
        return 1; 
    }
    std::string input;
    try {
        std::ifstream ifs(argv[1]);
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        input = buffer.str();
    }
    catch(const std::exception& e) {
        std::cerr << "Invalid file\n";
        return 1;
    }
    std::stringstream ss(input);
    std::string s;
    std::vector<std::string> tokens;
    while (std::getline(ss, s, '\n')) { 
        s.erase(std::remove_if(s.begin(), s.end(), isWhitespace), s.end());
        tokens.push_back(s);
    }
    Grammar g;
    g.tokens = tokens;
    Program* prog;
    try {
        prog = g.program(0);
        std::cout << *prog << "\n\n";
    } catch (fail& f) {
        std::cout << "parse error at token " << f.get() << "\n";
        prog = nullptr;
    }

    if (prog != nullptr) {
        //Note! Decl should only call it's own typeName
        for (Decl* g: prog->globals) { globals_map[g->name] = new TypeName(g->typeName()); }
        for (Struct* s: prog->structs) { 
            std::unordered_map<std::string, TypeName*> temp_map;
            for (Decl* f: s->fields) {
                temp_map[f->name] = new TypeName(f->typeName());
            }
            delta[s->name] = temp_map;
        }
        for (Decl* e: prog->externs) { globals_map[e->name] = new TypeName(e->typeName()); }
        for (Function* f: prog->functions) { //Adds type for function }
            if (f->name != "main") { globals_map[f->name] = new TypeName(f->typeName()); }
        }
        for (Function* f: prog->functions) { //Creating locals map
            if (f->name != "main") {
                std::unordered_map<std::string, TypeName*> temp_map;
                for (const auto& [name, type_name_pointer] : globals_map) {
                    temp_map.insert(std::make_pair(std::move(name), new TypeName(*type_name_pointer))); //Inserts all globals into function
                }
                for (Decl* p: f->params) { temp_map[p->name] = new TypeName(p->typeName()); }
                for (auto l: f->locals){ temp_map[l.first->name] = new TypeName(l.first->typeName()); }
                locals_map[f->name] = temp_map;
            }
        }   
        
        auto print_key_value = [](const auto& key, const auto& value)
        {
            std::cout << "Key:[" << key << "] Value:[" << value->get() << "]\n";
        };
        std::cout << "\nGlobals\n";
        for (auto g : globals_map){
            // std::cout << g.first << "\n";
            print_key_value(g.first, g.second);
        }
        std::cout << "\nStructs\n";
        for (auto s : delta){
            std::cout << s.first << ":\n";
            for (auto l: s.second) {
                print_key_value(l.first, l.second);
            }
        }
        std::cout << "\nLocals\n";
        for (auto f : locals_map){
            std::cout << f.first << ":\n";
            for (auto l: f.second) {
                print_key_value(l.first, l.second);
            }
        }
        std::cout << "\nFunctions\n";


        //Actual type checking, going through statements

        // for (Function* f: prog->functions) { //Creating locals map
        //     std::string function_name = f->name;
        //     for (Stmt* s: f->stmts) {
        //         s->typeCheck(locals_map[function_name], delta, f->rettyp->typeName(), false, errors_map, function_name); //defaults to not a loop
        //     }
        // } 

        // for (const auto& [error_type, errors] : errors_map) {
        //     std::sort(errors.begin(), errors.end());
        //     for (const auto& error : errors) {
        //         std::cout << error << "\n";
        //     }
        // }
        
    }


    return 0;
}