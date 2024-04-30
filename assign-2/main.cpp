#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "grammar.cpp"

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
    try {
        Program* prog = g.program(0);
        std::cout << *prog;
    } catch (fail& f) {
        std::cout << "parse error at token " << f.get() << "\n";
    }

    return 0;
}