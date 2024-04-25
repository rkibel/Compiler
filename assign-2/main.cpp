#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "grammar.cpp"

std::vector<std::string> tokens;

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
    unsigned int i = 0;
    while (i < tokens.size()) {
        try {
            auto [arg, itemp] = g.assign_or_call(i);
            std::cout << *arg << "\n";
            i = itemp;
        } catch(fail& f) {
            std::cout << "fail happened at i = " << i << "\n";
            i++;
        }
    }

    return 0;
}