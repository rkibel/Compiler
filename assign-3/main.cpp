#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "grammar.cpp"
#include "lir.cpp"
using namespace std;

// using ParamsReturnVal = pair<vector<Type*>, Type*>;
// using Gamma = unordered_map<string, Type*>;
// using Delta = unordered_map<string, Gamma>;
// using Errors = vector<string>;
// using FunctionsInfo = unordered_map<string, ParamsReturnVal>;
// using StructFunctionsInfo = unordered_map<string, FunctionsInfo>;

Gamma globals_map; // stores globals, externs, and functions
Delta delta; // struct name to (struct decl to type)
unordered_map<string, Gamma> locals_map; // function name to its locals (params and locals)
unordered_map<string, ParamsReturnVal> functions_map; // all functions to their funcInfo
unordered_map<string, FunctionsInfo> struct_functions_map; // struct name to (struct decl to funcInfo)
vector<string> errors_map;

bool isWhitespace(unsigned char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

void initializeMaps(AST::Program* prog) {
    for (AST::Decl* g: prog->globals) { 
        globals_map[g->name] = g->type;
        if (g->typeName().isPointerToFunction()) { functions_map[g->name] = g->funcInfo(); } // decl could be function pointer
    }
    for (AST::Struct* s: prog->structs) { 
        Gamma temp_map;
        // cout << "Adding for struct with name " << s->name;
        for (AST::Decl* f: s->fields) {
            temp_map[f->name] = f->type;
            // cout << " and for field " << f->name;
            if (f->typeName().isPointerToFunction()) {
                struct_functions_map[s->name][f->name] = f->funcInfo(); 
            }
        }
        delta[s->name] = temp_map;
        // cout << " ... added!\n";
    }

    for (AST::Decl* e: prog->externs) { 
        globals_map[e->name] = e->type; 
        if (e->name != "main") { functions_map[e->name] = e->funcInfo(); }
    }
    for (AST::Function* f: prog->functions) {
        globals_map[f->name] = f->functionType(); 
        functions_map[f->name] = f->funcInfo();
    }
    for (AST::Function* f: prog->functions) { // Creating locals map
        Gamma temp_map;
        for (const auto& [name, type_pointer] : globals_map) {
            temp_map.insert(make_pair(move(name), type_pointer)); // Inserts all globals into function's local map
        }
        for (AST::Decl* p: f->params) { 
            temp_map[p->name] = p->type; 
            if (p->typeName().isPointerToFunction()) { functions_map[p->name] = p->funcInfo(); } 
        }
        for (auto [decl, exp]: f->locals){ 
            temp_map[decl->name] = decl->type; 
            if (decl->typeName().isPointerToFunction()) { functions_map[decl->name] = decl->funcInfo(); } // decl could be function pointer
        }
        locals_map[f->name] = temp_map;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Incorrect number of args, should include json_file followed by token_stream\n"; 
        return 1; 
    }
    string input;
    try {
        ifstream ifs(argv[2]);
        stringstream buffer;
        buffer << ifs.rdbuf();
        input = buffer.str();
    }
    catch(const exception& e) {
        cerr << "Invalid file\n";
        return 1;
    }
    stringstream ss(input);
    string s;
    vector<string> tokens;
    while (getline(ss, s, '\n')) { 
        s.erase(remove_if(s.begin(), s.end(), isWhitespace), s.end());
        tokens.push_back(s);
    }
    Grammar g;
    g.tokens = tokens;
    Program* prog;
    try {
        prog = g.program(0);
    } catch (fail& f) {
        cout << "parse error at token " << f.get() << "\n";
        return 0;
    }
    initializeMaps(prog);
    /*prog->typeCheck(globals_map, errors_map, locals_map);
    sort(errors_map.begin(), errors_map.end());
    for (const auto& error : errors_map) {
        cout << error << "\n";
    }*/
    LIR::Program program(prog, locals_map);
    program.print();




    return 0;
}