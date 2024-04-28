#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "grammar.cpp"

std::vector<std::string> tokens;
//Declare type maps for prog
std::unordered_map<std::string, TypeName> globals_map; //only use global map to create gamma maps
std::unordered_map<std::string, std::unordered_map<std::string, TypeName>> delta;
std::unordered_map<std::string, std::unordered_map<std::string, TypeName>> gamma; //keys for gamma are function names, whose value is that function's gamma

//Gamma keys index into the locals map
TypeName NumRule(std::string gamma_key, int n);


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
    while (std::getline(ss, s, '\n')) { 
        tokens.push_back(s);
    }    
    Grammar g;
    g.tokens = tokens;
    Program* prog;
    try {
        prog = g.program(0);
        std::cout << *prog;
    } catch (fail& f) {
        std::cout << "parse error at token _\n";
    }

    if (prog != nullptr) {
        //Note! Decl should only call it's own type_name
        for (Decl* g: prog->globals) { globals_map[g->name] = g->type_name(); }
        for (Struct* s: prog->structs) { 
            std::unordered_map<std::string, TypeName> temp_map;
            for (Decl* f: s->fields) {
                temp_map[f->name] = f->type_name();
            }
            delta[s->name] = temp_map;
        }
        for (Decl* e: prog->externs) { globals_map[e->name] = e->type_name(); }
        for (Function* f: prog->functions) { //Adds type for function }
            if (f->name != "main") {
                globals_map[f->name] = f->type_name(); 
            }
        }
        for (Function* f: prog->functions) { //Creating locals map
            if (f->name != "main") {
                std::unordered_map<std::string, TypeName> temp_map;
                for (const auto& pair : globals_map) {
                    temp_map.insert(std::make_pair(std::move(pair.first), std::move(pair.second))); //Inserts all globals into function
                }
                for (Decl* p: f->params) { temp_map[p->name] = p->type_name(); }
                for (auto l: f->locals){ temp_map[l.first->name] = l.first->type_name(); }
                gamma[f->name] = temp_map;
            }
        }   
        
        /*auto print_key_value = [](const auto& key, const auto& value)
        {
            std::cout << "Key:[" << key << "] Value:[" << value.type_name << "]\n";
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
        for (auto f : gamma){
            std::cout << f.first << ":\n";
            for (auto l: f.second) {
                print_key_value(l.first, l.second);
            }
        }*/
        
    }


    return 0;
}