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

    std::string::size_type i = 0;
    std::string token = "Error";
    int length = 1;
    const std::string::size_type n = input.size();
    while (i < n) {
        const char c = input[i];
        if (isdigit(c)) {
            std::string::size_type iend = i+1;
            while (iend < n && isdigit(input[iend])) iend++;
            length = iend - i;
            token = "Num";
        }
        else if (isalpha(c)) {
            std::string::size_type iend = i+1;
            while (iend < n && (isalpha(input[iend]) || isdigit(input[iend]))) iend++;
            length = iend - i;
            static const std::unordered_map<std::string, std::string> tokenMap = {
                {"int", "Int"}, {"struct", "Struct"}, {"nil", "Nil"}, {"break", "Break"},
                {"continue", "Continue"}, {"return", "Return"}, {"if", "If"}, {"else", "Else"},
                {"while", "While"}, {"new", "New"}, {"let", "Let"}, {"extern", "Extern"}, {"fn", "Fn"}
            };
            auto it = tokenMap.find(input.substr(i, length));
            token = (it == tokenMap.end()) ? "Id" : it->second;
        }
        else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            std::string::size_type iend = i+1;
            while (iend < n && (input[iend] == ' ' || input[iend] == '\t' || input[iend] == '\n' || input[iend] == '\r')) iend++;
            length = iend - i;
            token = "whitespace";
        }
        else if (c == '/') {
            if (i+1 < input.length() && input[i+1] == '/') {
                std::string::size_type iend = i+2;
                while (iend < input.length() && input[iend] != '\n') iend++;
                length = iend - i;
                token = "c++-comment";
            }
            else if (i+1 < input.length() && input[i+1] == '*') {
                std::string::size_type iend = i+2;
                while (iend < n && !(iend < n-1 && input[iend] == '*' && input[iend+1] == '/')) iend++;
                if (iend < n-1 && input[iend] == '*' && input[iend+1] == '/') {
                    length = iend - i + 2;
                    token = "c-comment";
                }
                else {
                    length = iend - i;
                    token = "unclosed-c-comment";
                }
            }
            else { token = "Slash"; }
        }
        switch(c) {
            case '&': token = "Address"; break;
            case ':': token = "Colon"; break;
            case ';': token = "Semicolon"; break;
            case ',': token = "Comma"; break;
            case '_': token = "Underscore"; break;
            case '+': token = "Plus"; break;
            case '-': 
                if (i+1 < n && input[i+1] == '>') { length = 2; token = "Arrow"; }
                else { token = "Dash"; }
                break;
            case '*': token = "Star"; break;
            case '=': 
                if (i+1 < n && input[i+1] == '=') { length = 2; token = "Equal"; }
                else { token = "Gets"; }
                break;            
            case '!': 
                if (i+1 < n && input[i+1] == '=') { length = 2; token = "NotEq"; }
                break;
            case '<':
                if (i+1 < n && input[i+1] == '=') { length = 2; token = "Lte"; }
                else { token = "Lt"; }
                break;
            case '>':
                if (i+1 < n && input[i+1] == '=') { length = 2; token = "Gte"; }
                else { token = "Gt"; }
                break;
            case '.': token = "Dot"; break;
            case '(': token = "OpenParen"; break;
            case ')': token = "CloseParen"; break;
            case '[': token = "OpenBracket"; break;
            case ']': token = "CloseBracket"; break;
            case '{': token = "OpenBrace"; break;
            case '}': token = "CloseBrace"; break;
        }
        
        if (token == "Id" || token == "Num")  std::cout << token << "(" << input.substr(i, length) << ")" << std::endl;
        else if (token != "whitespace" && token != "c++-comment" && token != "c-comment" && token != "unclosed-c-comment") std::cout << token << std::endl;
        i += length;
        length = 1;
        token = "Error";
    }
    return 0;
}