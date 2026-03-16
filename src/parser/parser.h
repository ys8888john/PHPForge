#ifndef PARSER_H
#define PARSER_H

#include "../lexer/token.h"
#include "../ast/program.h"
#include "../ast/statement.h"
#include "../ast/ASTNode.h"
#include "../ast/functionDecl.h"
#include <vector>
#include <unordered_set>

namespace PHPForge {
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // 主入口： 解析整个程序
    std::unique_ptr<Program> parse();

    // 调试：打印当前token
    void debug();
private:
    // 向前看token
    const Token& peek(int offset = 0) const;
    const Token& current();

    // 消费token
    const Token& consume();
    const Token& consume(TokenType type, const std::string& expected = "");

    // 匹配检查
    bool check(TokenType type);
    bool check(TokenType type, const std::string& value);
    bool match(TokenType type);
    bool match(TokenType type, const std::string& value);

    // 错误回复
    void synchronize();

    // 语法规则
    // 程序规则
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseDeclaration();

    // 声明
    std::unique_ptr<ASTNode> parseDeclareStmt();
    std::unique_ptr<ASTNode> parseFunctionDecl();
    std::unique_ptr<ASTNode> parseVariableDecl();

    // 语句
    std::unique_ptr<ASTNode> parseReturnStmt();
    std::unique_ptr<ASTNode> parseEchoStmt();
    std::unique_ptr<ASTNode> parseExpressionStmt();
    std::unique_ptr<ASTNode> parseBlockStmt();

    // 表达式
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseAssignment();
    std::unique_ptr<ASTNode> parseEquality();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseFactor();
    std::unique_ptr<ASTNode> parsePrimary();

    // 辅助
    std::unique_ptr<ASTNode> parseCall(std::unique_ptr<ASTNode> callee);
    std::unique_ptr<ASTNode> parseArguments();
    std::vector<std::unique_ptr<Parameter>> parseParameters();
    std::string parseTypeHint();

    // 工具方法
    bool isAtEnd();
    bool isDeclaration();
    bool isStatement();

private:
    std::vector<Token> tokens;
    mutable size_t currentPos = 0;

    // 同步集合：用于错误恢复
    static const std::unordered_set<TokenType> SYNCHRONIZATION_TOKENS;
};
} // namespace PHPForge

#endif // PARSER_H