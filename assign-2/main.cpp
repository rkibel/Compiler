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

Gamma globals_map;
Delta delta;
std::unordered_map<std::string, Gamma> locals_map;
std::unordered_map<std::string, ParamsReturnVal> functions_map;
std::unordered_map<std::string, FunctionsInfo> struct_functions_map;
std::vector<std::string> errors_map;

const bool isWhitespace(unsigned char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

void initializeMaps(AST::Program* prog) {
    for (AST::Decl* g: prog->globals) { 
        globals_map[g->name] = new TypeName(g->typeName());
        if (g->typeName().isPointerToFunction()) { functions_map[g->name] = g->funcInfo(); } // decl could be function pointer
    }
    for (AST::Struct* s: prog->structs) { 
        Gamma temp_map;
        for (AST::Decl* f: s->fields) {
            temp_map[f->name] = new TypeName(f->typeName());
            if (f->typeName().isPointerToFunction()) { 
                struct_functions_map[s->name][f->name] = f->funcInfo(); 
            }
        }
        delta[s->name] = temp_map;
    }

    for (AST::Decl* e: prog->externs) { 
        globals_map[e->name] = new TypeName(e->typeName()); 
        if (e->name != "main") { functions_map[e->name] = e->funcInfo(); }
    }
    for (AST::Function* f: prog->functions) {
        globals_map[f->name] = new TypeName(f->typeName()); 
        functions_map[f->name] = f->funcInfo();
    }
    for (AST::Function* f: prog->functions) { // Creating locals map
        Gamma temp_map;
        for (const auto& [name, type_name_pointer] : globals_map) {
            temp_map.insert(std::make_pair(std::move(name), new TypeName(*type_name_pointer))); // Inserts all globals into function
        }
        for (AST::Decl* p: f->params) { 
            temp_map[p->name] = new TypeName(p->typeName()); 
            if (p->typeName().isPointerToFunction()) { functions_map[p->name] = p->funcInfo(); } 
        }
        for (auto [decl, exp]: f->locals){ 
            temp_map[decl->name] = new TypeName(decl->typeName()); 
            if (decl->typeName().isPointerToFunction()) { functions_map[decl->name] = decl->funcInfo(); } // decl could be function pointer
        }
        locals_map[f->name] = temp_map;
    }
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
    AST::Program* prog;
    try {
        prog = g.program(0);
        std::cout << *prog << "\n\n";
    } catch (fail& f) {
        std::cout << "parse error at token " << f.get() << "\n";
        prog = nullptr;
        return 0;
    }
    initializeMaps(prog);
    prog->typeCheck(globals_map, errors_map, locals_map);
    std::sort(errors_map.begin(), errors_map.end());
    for (const auto& error : errors_map) {
        std::cout << error << "\n";
    }


    return 0;
}