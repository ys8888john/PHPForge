#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "src/lexer/lexer.h"
#include "src/parser/parser.h"
#include "src/semantic/semanticAnalyzer.h"
#include "src/codegen/codegen.h"

bool readFile(const std::string &filename, std::vector<char> &buffer)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "error: can't open php file path" << std::endl;
        return false;
    }

    size_t size = file.tellg();
    if (size == static_cast<size_t>(-1))
    {
        std::cerr << "error: can't get php file size" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    buffer.resize(size);

    if (!file.read(buffer.data(), size))
    {
        std::cerr << "error: can't read php file" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "error: please set php file path" << std::endl;
        return 1;
    }
    std::string phpFilePath = argv[1];
    std::vector<char> buffer;
    if (!readFile(phpFilePath, buffer))
    {
        return 1;
    }
    try
    {
        // 1. 词法分析
        PHPForge::Lexer lexer(buffer);
        auto tokens = lexer.tokenize();
        lexer.dump(tokens);

        // 2. 语法分析
        PHPForge::Parser parser(tokens);
        auto ast = parser.parse();
        if (!ast)
        {
            return 1;
        }
        ast->dump();
        std::cout << "\n=== 开始语义分析 ===\n" << std::endl;

        // 3. 语义分析
        PHPForge::SemanticAnalyzer semanticAnalyzer;
        semanticAnalyzer.analyze(ast.get());
        semanticAnalyzer.dumpSymbolTable();

        if (semanticAnalyzer.hasErrors())
        {
            std::cerr << "\nSemantic analysis failed with "
                      << semanticAnalyzer.getErrors().size() << " errors\n";
            return 1;
        }

        std::cout << "Semantic analysis completed successfully!\n" << std::endl;

        // 4. 代码生成 (LLVM IR)
        std::cout << "=== 开始生成 LLVM IR ===\n" << std::endl;
        PHPForge::CodeGenerator codegen;
        codegen.generate(ast.get());
        codegen.dumpIR();

        // 输出 IR 到 .ll 文件
        std::string irFile = phpFilePath.substr(0, phpFilePath.rfind('.')) + ".ll";
        if (codegen.writeIRToFile(irFile)) {
            std::cout << "\nIR written to: " << irFile << std::endl;
        }

        // 5. JIT 执行
        std::cout << "\n=== 开始 JIT 执行 ===" << std::endl;
        int exitCode = codegen.jit();
        std::cout << "\n=== JIT 执行完成 (exit code: " << exitCode << ") ===\n" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
