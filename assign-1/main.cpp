#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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
    int length = 1;
    std::string token = "Error";
    std::vector<std::string> res;
    while (i < input.size()) {
        if (isdigit(input[i])) {
            std::string::size_type iend = i+1;
            while (iend < input.size() && isdigit(input[iend])) iend++;
            length = iend - i;
            token = "Num";
        }
        else if (isalpha(input[i])) {
            std::string::size_type iend = i+1;
            while (iend < input.size() && (isalpha(input[iend]) || isdigit(input[iend]))) {
                iend++;
            }
            length = iend - i;
            if (input.substr(i, length) == "int") token = "Int";
            else if (input.substr(i, length) == "struct") token = "Struct";
            else if (input.substr(i, length) == "nil") token = "Nil";
            else if (input.substr(i, length) == "break") token = "Break";
            else if (input.substr(i, length) == "continue") token = "Continue";
            else if (input.substr(i, length) == "return") token = "Return";
            else if (input.substr(i, length) == "if") token = "If";
            else if (input.substr(i, length) == "else") token = "Else";
            else if (input.substr(i, length) == "while") token = "While";
            else if (input.substr(i, length) == "new") token = "New";
            else if (input.substr(i, length) == "let") token = "Let";
            else if (input.substr(i, length) == "extern") token = "Extern";
            else if (input.substr(i, length) == "fn") token = "Fn";
            else token = "Id";
        }
        else if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r') {
            std::string::size_type iend = i+1;
            while (iend < input.size() && (input[iend] == ' ' || input[iend] == '\t' || input[iend] == '\n' || input[iend] == '\r')) iend++;
            length = iend - i;
            token = "whitespace";
        }
        else if (input[i] == '/') {
            if (i+1 < input.length() && input[i+1] == '/') {
                std::string::size_type iend = i+2;
                while (iend < input.length() && input[iend] != '\n') iend++;
                length = iend - i;
                token = "c++-comment";
            }
            else if (i+1 < input.length() && input[i+1] == '*') {
                std::string::size_type iend = i+2;
                while (iend < input.size() && !(iend < input.size() - 1 && input[iend] == '*' && input[iend+1] == '/')) iend++;
                if (iend < input.size()-1 && input[iend] == '*' && input[iend+1] == '/') {
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
        switch(input[i]) {
            case '&': token = "Address"; break;
            case ':': token = "Colon"; break;
            case ';': token = "Semicolon"; break;
            case ',': token = "Comma"; break;
            case '_': token = "Underscore"; break;
            case '+': token = "Plus"; break;
            case '-': 
                if (i+1 < input.size() && input[i+1] == '>') {
                    length = 2;
                    token = "Arrow";
                }
                else { token = "Dash"; }
                break;
            case '*': token = "Star"; break;
            case '=': 
                if (i+1 < input.size() && input[i+1] == '=') {
                    length = 2;
                    token = "Equal";
                }
                else { token = "Gets"; }
                break;            
            case '!': 
                if (i+1 < input.size() && input[i+1] == '=') {
                    length = 2;
                    token = "NotEq";
                }
                break;
            case '<':
                if (i+1 < input.size() && input[i+1] == '=') {
                    length = 2;
                    token = "Lte";
                }
                else { token = "Lt"; }
                break;
            case '>':
                if (i+1 < input.size() && input[i+1] == '=') {
                    length = 2;
                    token = "Gte";
                }
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
        

        if (token == "Id" || token == "Num") {
            res.push_back(token + "(" + input.substr(i, length) + ")");
        }
        else if (token != "whitespace" && token != "c++-comment" && token != "c-comment" && token != "unclosed-c-comment") {
            res.push_back(token);
        }
        i += length;
        length = 1;
        token = "Error";
    }
    for (std::string str: res) {
        std::cout << str << std::endl;
    }
    return 0;
}