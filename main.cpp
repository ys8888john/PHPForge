#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "src/lexer/lexer.h"
#include "src/parser/parser.h"

bool readFile(const std::string& filename, std::vector<char>& buffer) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "error: can't open php file path" << std::endl;
        return false;
    }

    size_t size = file.tellg();
    if (size == static_cast<size_t>(-1)) {
        std::cerr << "error: can't get php file size" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    buffer.resize(size);

    if (!file.read(buffer.data(), size)) {
        std::cerr << "error: can't read php file" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char** argv){
    if (argc < 2) {
        std::cerr << "error: please set php file path" << std::endl;
        return 1;
    }
    std::string phpFilePath = argv[1];
    std::vector<char> buffer;
    if (!readFile(phpFilePath, buffer)) {
        return 1;
    }
    try {
        // 1. 词法分析
        PHPForge::Lexer lexer(buffer);
        auto tokens = lexer.tokenize();
        lexer.dump(tokens);

        // 2. 语法分析
        PHPForge::Parser parser(tokens);
        auto ast = parser.parse();
        if (ast) {
            ast->dump();
        }

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
