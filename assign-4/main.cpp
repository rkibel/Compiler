#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "lir.cpp"
#include "json.hpp"

// for convenience
using json = nlohmann::json;


int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename.json>" << std::endl;
        return 1;
    }

    // Open the file
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }

    // Parse JSON data from the file
    json data;
    file >> data;

    // Close the file
    file.close();

    std::cout << "Parsed JSON data:" << std::endl;
    std::cout << data.dump(2) << std::endl;
    // Program prog;
    // cout << prog;
    return 0;
}