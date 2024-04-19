#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

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
    std::vector<std::string> tokens;
    std::string s;
    while (std::getline(ss, s, '\n')) { 
        tokens.push_back(s);
    }
 
    for (unsigned int i = 0; i < tokens.size(); i++) {
        std::cout << tokens[i] << std::endl;
    }


    return 0;
}