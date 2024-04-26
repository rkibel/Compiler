#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>

bool isWhitespace(unsigned char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

int main(int argc, char *argv[]) {
    std::string s;
    std::ifstream ifs(argv[1]);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    s = buffer.str();
    s.erase(std::remove_if(s.begin(), s.end(), isWhitespace), s.end());
    std::cout << s;

    return 0;
}
