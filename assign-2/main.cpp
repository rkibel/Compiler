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
using Errors = std::vector<std::string>;
using FunctionsInfo = std::unordered_map<std::string, ParamsReturnVal>;
using StructFunctionsInfo = std::unordered_map<std::string, FunctionsInfo>;


std::vector<std::string> tokens;
//Declare type maps for prog
Gamma globals_map; //used for creating local maps and for main function
Delta delta;
std::unordered_map<std::string, Gamma> locals_map; //keys for gamma are (non-main) function names, whose value is that function's gamma
FunctionsInfo functions_map;
StructFunctionsInfo struct_functions_map;
Errors errors_map;

bool isWhitespace(unsigned char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

extern const bool isFunction(const std::string& type) {
    return type[0] == '(' || type.find("&(") != std::string::npos;
}
extern const bool isFunctionNotPointer(const std::string& type) {
    return type[0] == '(';
}
extern const bool isStruct(const std::string& type) {
    return type[0] != '&' && type.find('(') == std::string::npos && type.substr(0,3) != "int";
}
extern const bool isValidFieldAcesss(const std::string& type) {
    return type[0] == '&' && type[1] != '(' && type[1] != '&' && type.substr(0,5) != "&int"; //one extra for substr in case type called inty or something
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
        for (Decl* g: prog->globals) { 
            globals_map[g->name] = new TypeName(g->typeName());
            if (isFunction(g->typeName().get())) { functions_map[g->name] = g->funcInfo(); } //decl could be function pointer
        }
        for (Struct* s: prog->structs) { 
            Gamma temp_map;
            for (Decl* f: s->fields) {
                // if (f->name == "fun") { // std::cout << s->name << " YESSIR\n"; }
                temp_map[f->name] = new TypeName(f->typeName());
                // std::cout << "Type name here: " << f->typeName().get() << "\n";
                if (isFunction(f->typeName().get())) { 
                    // std::cout << "Number of parameters: " << f->funcInfo().params.size() << "\n";
                    struct_functions_map[s->name][f->name] = f->funcInfo(); 
                }
            }
            delta[s->name] = temp_map;
        }
        for (Decl* e: prog->externs) { 
            globals_map[e->name] = new TypeName(e->typeName()); 
            if (e->name != "main") { functions_map[e->name] = e->funcInfo(); }
        }
        for (Function* f: prog->functions) {
            globals_map[f->name] = new TypeName(f->typeName()); 
            functions_map[f->name] = f->funcInfo();
        }
        for (Function* f: prog->functions) { //Creating locals map
            Gamma temp_map;
            for (const auto& [name, type_name_pointer] : globals_map) {
                temp_map.insert(std::make_pair(std::move(name), new TypeName(*type_name_pointer))); //Inserts all globals into function
            }
            // for (Struct* s: prog->structs) { //Do above struct and function earlier so each gamma has full global information
            //     temp_map[s->name] = new TypeName(s->typeName());
            // }
            for (Decl* p: f->params) { 
                temp_map[p->name] = new TypeName(p->typeName()); 
                if (isFunction(p->typeName().get())) { functions_map[p->name] = p->funcInfo(); } 
            }
            for (auto [decl, exp]: f->locals){ 
                temp_map[decl->name] = new TypeName(decl->typeName()); 
                if (isFunction(decl->typeName().get())) { functions_map[decl->name] = decl->funcInfo(); } //decl could be function pointer
            }
            locals_map[f->name] = temp_map;
        }   
        
        
        auto print_key_value = [](const auto& key, const auto& value)
        {
            std::cout << "\tKey:[" << key << "] Value:[" << value->get() << "]\n";
        };
        // // std::cout << "\nGlobals\n";
        // for (auto g : globals_map){
        //     // // std::cout << g.first << "\n";
        //     print_key_value(g.first, g.second);
        // }
        // // std::cout << "\nStructs\n";
        // for (auto s : delta){
        //     // std::cout << s.first << ":\n";
        //     for (auto l: s.second) {
        //         print_key_value(l.first, l.second);
        //     }
        // }
        // // std::cout << "\nLocals\n";
        // for (auto f : locals_map){
        //     // std::cout << f.first << ":\n";
        //     for (auto l: f.second) {
        //         print_key_value(l.first, l.second);
        //     }
        // }
        // // std::cout << "\nFunctions\n";
        // for (auto f : locals_map){
        //     // std::cout << f.first << ":\n";
        //     for (auto l: f.second) {
        //         print_key_value(l.first, l.second);
        //     }
        // }


        // // std::cout << "Functions map\n"; //Seems correct
        // for (const auto& entry : functions_map) {
        //     // std::cout << entry.first << std::endl; // entry.first contains the key (function name)
        //     // // std::cout << entry.second.rettyp->typeName().get() << std::endl;
        // }
        // // std::cout << "Moving on to real type checking\n\n";
        
        // Actual type checking, going through statements
        // for (Function* f: prog->functions) { //Creating locals map
        //     std::string function_name = f->name;
        //     for (Stmt* s: f->stmts) {
        //         s->typeCheck(locals_map[function_name], f, false, errors_map); //defaults to not a loop
        //     }
        // } 

        bool program_correct = prog->typeCheck(globals_map, errors_map); //Even if incorrect program, we'll still print out the errors

        //Sorting errors
        std::sort(errors_map.begin(), errors_map.end());

        //Printing out errors
        for (const auto& error : errors_map) {
            std::cout << error << "\n";
        }
    }
    return 0;
}