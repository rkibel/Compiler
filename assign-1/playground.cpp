#include <iostream>
#include <string>

void printPrefix(const std::string& x, const std::string& suffix) {
    size_t suffixIndex = x.find(suffix);
    if (suffixIndex != std::string::npos) {
        std::cout << "Prefix of '" << x << "' given the suffix '" << suffix << "': ";
        std::cout << x.substr(0, suffixIndex) << std::endl;
    } else {
        std::cout << "Suffix not found in the string." << std::endl;
    }
}

int main() {
    std::string x = "parked";
    std::string suffix = "";
    // printPrefix(x, suffix);
    std::cout << x.substr(6) << "\n";
    std::cout << (x.substr(6) == "") << "\n";
    return 0;
}
