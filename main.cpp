#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "lexer.h"

bool readFile(const std::string& filename, std::vector<char>& buffer) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "error: can't open php file path" << std::endl;
        return false;
    }

    size_t size = file.tellg();
    if (size == -1) {
        std::cerr << "error: can't get php file size" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    buffer.resize(size);

    if (!file.read(buffer.data(), size)) {
        std::cerr << "error: can't read php file" << std::endl;
        return false;
    }
}

int main(int argc, char** argv){
    if (argc < 2) {
        std::cerr << "error: please set php file path" << std::endl;
        return 1;
    }
    std::string phpFilePath = argv[1];
    std::vector<char> buffer;
    if (readFile(phpFilePath, buffer)) {
        return 1;
    }
    try { 
        PHPForge::Lexer lexer(buffer); 
        auto tokens = lexer.tokenize();

        // 输出所有token
        size_t meaningful_tokens = 0, token_count = 0;
        for (const auto& token : tokens) {
            // 跳过空白、注释和EOF
            if (token.is(PHPForge::TokenType::T_WHITESPACE) || token.is(PHPForge::TokenType::T_EOF)) {
                continue;
            }

            std::cout << "token type: " << tokenTypeToString(token.getType()) <<  ", token value: " << token.getValue() << "\n";
            meaningful_tokens++;
            token_count++;
        }
        
        std::cout << "\n统计信息:\n";
        std::cout << "================\n";
        std::cout << "总token数: " << token_count << "\n";
        std::cout << "有效token数: " << meaningful_tokens << "\n";

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
