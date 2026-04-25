#include "parser.h"
#include "../ast/expression.h"
#include "../ast/statement.h"

namespace PHPForge {

// 同步token集合
const std::unordered_set<TokenType> Parser::SYNCHRONIZATION_TOKENS = {
    TokenType::T_FUNCTION,
    TokenType::T_RETURN,
    TokenType::T_ECHO,
    TokenType::T_SEMICOLON,
    TokenType::T_RBRACE
};

// 构造函数
Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), currentPos(0) {}

// 主解析方法
std::unique_ptr<Program> Parser::parse() {
    try {
        auto program = std::make_unique<Program>();
        while (check(TokenType::T_OPEN_TAG)) {
            consume();
        }

        while (!isAtEnd()) {
            if (check(TokenType::T_WHITESPACE)) {
                consume();
                continue;
            }

            try {
                if (isDeclaration()) {
                    auto decl = parseDeclaration();
                    if (decl) program->addStatement(std::move(decl));
                } else if (isStatement()) {
                    auto stmt = parseStatement();
                    if (stmt) program->addStatement(std::move(stmt));
                } else {
                    throw std::runtime_error(
                        "Unexpected token '" + peek().getValue() + "' at line " +
                        std::to_string(peek().getLine()) + ", column " +
                        std::to_string(peek().getColumn())
                    );
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                synchronize();
                while (check(TokenType::T_WHITESPACE)) { consume(); }
            }
        }

        return program;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return nullptr;
    }
}

// 解析声明
std::unique_ptr<ASTNode> Parser::parseDeclaration() {
    if (check(TokenType::T_DECLARE)) {
        return parseDeclareStmt();
    } else if (check(TokenType::T_FUNCTION)) {
        return parseFunctionDecl();
    } else if (check(TokenType::T_VARIABLE)) {
        return parseVariableDecl();
    }

    return nullptr;
}

// 解析declare语句
std::unique_ptr<ASTNode> Parser::parseDeclareStmt() {
    int line = current().getLine();
    int column = current().getColumn();

    consume(); // 消费 T_DECLARE

    consume(TokenType::T_LPAREN, "期望 '(' 在 declare 之后");

    // 解析 strict_types 声明
    if (!check(TokenType::T_STRICT_TYPES)) {
        throw std::runtime_error(
            "期望 'strict_types' 在 declare 之后"
        );
    }

    auto strict_types = consume();
    std::string directive = strict_types.getValue();
    consume(TokenType::T_ASSIGN, "期望 '='");

    // 解析值
    if (!check(TokenType::T_NUMBER)) {
        throw std::runtime_error(
            "期望数字"
        );
    }

    auto valueToken = consume();
    int value = std::stoi(valueToken.getValue());

    consume(TokenType::T_RPAREN, "期望 ')'");
    consume(TokenType::T_SEMICOLON, "期望 ';'");

    return std::make_unique<DeclareStmt>(directive, value, line, column);
}

// 解析函数声明
std::unique_ptr<ASTNode> Parser::parseFunctionDecl() {
    consume(); // 消费 T_FUNCTION

    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    // 函数名
    if (!check(TokenType::T_IDENTIFIER)) {
        throw std::runtime_error("期望函数名");
    }

    auto functionName = consume();

    int line = functionName.getLine();
    int column = functionName.getColumn();

    // 参数列表
    consume(TokenType::T_LPAREN, "期望 '(' 在函数名之后");

    std::vector<std::unique_ptr<Parameter>> parameters;
    if (!check(TokenType::T_RPAREN)) {
        parameters = parseParameters();
    }

    consume(TokenType::T_RPAREN, "期望 ')' 在参数列表之后");

    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    // 返回类型
    std::string returnType = "void";
    if (match(TokenType::T_COLON)) {
        returnType = parseTypeHint();
    }

    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    // 函数体
    consume(TokenType::T_LBRACE, "期望 '{' 在函数声明之后");
    auto bodyNode = parseBlockStmt();
    // parseBlockStmt 已经消费了 T_RBRACE

    // 将 ASTNode 转换为 BlockStmt
    BlockStmt* body = static_cast<BlockStmt*>(bodyNode.get());
    auto bodyUnique = std::unique_ptr<BlockStmt>(body);
    bodyNode.release();  // 防止双重删除

    return std::make_unique<FunctionDecl>(
        functionName.getValue(),
        returnType,
        std::move(parameters),
        std::move(bodyUnique),
        line,
        column
    );
}

// 解析参数列表
std::vector<std::unique_ptr<Parameter>> Parser::parseParameters() {
    std::vector<std::unique_ptr<Parameter>> params;

    do {
        // 跳过空白
        while (check(TokenType::T_WHITESPACE)) {
            consume();
        }

        // 类型提示（可选）
        std::string typeHint = "";
        if (check(TokenType::T_INT) || check(TokenType::T_IDENTIFIER)) {
            typeHint = parseTypeHint();
        }

        // 跳过空白
        while (check(TokenType::T_WHITESPACE)) {
            consume();
        }

        // 参数名
        if (!check(TokenType::T_VARIABLE)) {
            throw std::runtime_error("期望参数名");
        }

        auto paramName = consume();

        // 去掉$前缀
        std::string name = paramName.getValue();
        if (!name.empty() && name[0] == '$') {
            name = name.substr(1);
        }

        params.push_back(std::make_unique<Parameter>(name, typeHint, paramName.getLine(), paramName.getColumn()));

    } while (match(TokenType::T_COMMA));

    return params;
}

// 解析类型提示
std::string Parser::parseTypeHint() {
    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    if (check(TokenType::T_INT)) {
        auto typeToken = consume();
        return typeToken.getValue();
    } else if (check(TokenType::T_IDENTIFIER)) {
        // 自定义类型
        auto typeToken = consume();
        return typeToken.getValue();
    }

    throw std::runtime_error(
        "期望类型提示" +
        std::to_string(current().getLine()) + ":" + std::to_string(current().getColumn())
    );
}

// 解析语句
std::unique_ptr<ASTNode> Parser::parseStatement() {
    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }
    
    if (check(TokenType::T_RETURN)) {
        return parseReturnStmt();
    } else if (check(TokenType::T_ECHO)) {
        return parseEchoStmt();
    } else if (check(TokenType::T_LBRACE)) {
        return parseBlockStmt();
    } else if (check(TokenType::T_IF)) {
        return parseIfStmt();
    } else if (check(TokenType::T_FOR)) {
        return parseForStmt();
    } else if (check(TokenType::T_SINGLE_QUOTE)) {
        // TODO: 解析单引号字符串
        return nullptr;
    } else if (check(TokenType::T_QUOTE)) {
        // TODO: 解析双引号字符串
        return nullptr;
    } else {
        return parseExpressionStmt();
    }
    return nullptr;
}

// 解析返回语句
std::unique_ptr<ASTNode> Parser::parseReturnStmt() {
    auto returnToken = consume();

    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    std::unique_ptr<ASTNode> expr = nullptr;
    if (!check(TokenType::T_SEMICOLON)) {
        expr = parseExpression();
    }

    consume(TokenType::T_SEMICOLON, "期望 ';' 在 return 之后");
    return std::make_unique<ReturnStmt>(std::move(expr), returnToken.getLine(), returnToken.getColumn());
}

// 解析echo语句
std::unique_ptr<ASTNode> Parser::parseEchoStmt() {
    auto echoToken = consume();

    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    auto expr = parseExpression();
    consume(TokenType::T_SEMICOLON, "期望 ';'");

    return std::make_unique<EchoStmt>(std::move(expr), echoToken.getLine(), echoToken.getColumn());
}

// 解析代码块
std::unique_ptr<ASTNode> Parser::parseBlockStmt() {
    int line = current().getLine();
    int column = current().getColumn();
    
    auto block = std::make_unique<BlockStmt>(line, column);
    
    while (!isAtEnd() && !check(TokenType::T_RBRACE)) {
        if (check(TokenType::T_WHITESPACE)) {
            consume();
            continue;
        }
        
        auto stmt = parseStatement();
        if (stmt) {
            block->addStatement(std::move(stmt));
        }
    }
    
    consume(TokenType::T_RBRACE, "期望 '}'");
    return block;
}

// 解析if语句
std::unique_ptr<ASTNode> Parser::parseIfStmt() {
    auto ifToken = consume();
    int line = ifToken.getLine();
    int column = ifToken.getColumn();
    
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }

    consume(TokenType::T_LPAREN, "期望 '('");

    auto condition = parseExpression();

    consume(TokenType::T_RPAREN, "期望 ')'");

    while(check(TokenType::T_WHITESPACE)) {
        consume();
    }

    consume(TokenType::T_LBRACE, "期望 '{'");

    auto thenBranch = parseBlockStmt();

    // TODO: parse else or if else branch
    std::unique_ptr<ASTNode> elseBranch = nullptr;

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line, column);
}

// 解析for语句
std::unique_ptr<ASTNode> Parser::parseForStmt() {
    auto forToken = consume(); // consume T_FOR
    int line = forToken.getLine();
    int column = forToken.getColumn();

    while (check(TokenType::T_WHITESPACE)) { consume(); }
    consume(TokenType::T_LPAREN, "期望 '(' 在 for 之后");

    // Init
    while (check(TokenType::T_WHITESPACE)) { consume(); }
    std::unique_ptr<ASTNode> init = nullptr;
    if (!check(TokenType::T_SEMICOLON)) {
        init = parseExpression();
    }
    consume(TokenType::T_SEMICOLON, "期望 ';' 在 for 初始化之后");

    // Condition
    while (check(TokenType::T_WHITESPACE)) { consume(); }
    std::unique_ptr<ASTNode> condition = nullptr;
    if (!check(TokenType::T_SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::T_SEMICOLON, "期望 ';' 在 for 条件之后");

    // Update
    while (check(TokenType::T_WHITESPACE)) { consume(); }
    std::unique_ptr<ASTNode> update = nullptr;
    if (!check(TokenType::T_RPAREN)) {
        update = parseExpression();
    }
    consume(TokenType::T_RPAREN, "期望 ')' 在 for 之后");

    while (check(TokenType::T_WHITESPACE)) { consume(); }
    consume(TokenType::T_LBRACE, "期望 '{' 在 for 之后");

    auto body = parseBlockStmt();

    return std::make_unique<ForStmt>(std::move(init), std::move(condition), std::move(update), std::move(body), line, column);
}

// ============ 优先级表（数值越大优先级越高）============
namespace {
    // 前缀token没有左结合性，返回0
    inline int getPrecedence(TokenType type) {
        switch (type) {
            case TokenType::T_ASSIGN:       return 1;   // =
            case TokenType::T_EQUAL_EQUAL:   // ==
            case TokenType::T_NOT_EQUAL:     return 3;   // !=
            case TokenType::T_LESS:          // <
            case TokenType::T_GREATER:       // >
            case TokenType::T_LESS_EQUAL:    // <=
            case TokenType::T_GREATER_EQUAL: return 4;   // >=
            case TokenType::T_ADD:           // +
            case TokenType::T_MINUS:         return 5;
            default: return 0;
        }
    }
}

// 解析表达式（Pratt Parser：单函数 + 优先级爬升）
std::unique_ptr<ASTNode> Parser::parseExpression(int minPrecedence) {
    auto left = parsePrimary();

    while (!isAtEnd()) {
        while (check(TokenType::T_WHITESPACE)) { consume(); }

        int prec = getPrecedence(current().getType());
        if (prec <= minPrecedence) break;

        auto opToken = consume();
        std::string op = opToken.getValue();

        while (check(TokenType::T_WHITESPACE)) { consume(); }

        auto right = parseExpression(prec);

        left = std::make_unique<BinaryExpr>(
            std::move(left), op, std::move(right),
            opToken.getLine(), opToken.getColumn()
        );
    }

    return left;
}

// 解析基本表达式（原子/前缀）
std::unique_ptr<ASTNode> Parser::parsePrimary() {
    while (check(TokenType::T_WHITESPACE)) { consume(); }

    if (match(TokenType::T_NUMBER)) {
        auto token = peek(-1);
        return std::make_unique<LiteralExpr>(
            token.getValue(), "int",
            token.getLine(), token.getColumn()
        );
    }
    if (match(TokenType::T_VARIABLE)) {
        auto token = peek(-1);
        std::string name = token.getValue();
        if (!name.empty() && name[0] == '$') { name = name.substr(1); }
        return std::make_unique<VariableExpr>(
            name, token.getLine(), token.getColumn()
        );
    }
    if (match(TokenType::T_IDENTIFIER)) {
        auto token = peek(-1);
        auto identifier = std::make_unique<IdentifierExpr>(
            token.getValue(), token.getLine(), token.getColumn()
        );
        if (check(TokenType::T_LPAREN)) {
            return parseCall(std::move(identifier));
        }
        return identifier;
    }
    if (match(TokenType::T_LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::T_RPAREN, "期望 ')'");
        return expr;
    }
    if (match(TokenType::T_TRUE) || match(TokenType::T_FALSE)) {
        auto token = peek(-1);
        return std::make_unique<LiteralExpr>(token.getValue(), "bool", token.getLine(), token.getColumn());
    }
    if (match(TokenType::T_STRING)) {
        auto token = peek(-1);
        return std::make_unique<LiteralExpr>(token.getValue(), "string", token.getLine(), token.getColumn());
    }

    throw std::runtime_error(
        "期望表达式" +
        std::to_string(current().getLine()) + ":" + std::to_string(current().getColumn())
    );
}

// 解析函数调用
std::unique_ptr<ASTNode> Parser::parseCall(std::unique_ptr<ASTNode> callee) {
    int line = current().getLine();
    int column = current().getColumn();

    consume(TokenType::T_LPAREN, "期望 '('");
    std::vector<std::unique_ptr<ASTNode>> args;

    if (!check(TokenType::T_RPAREN)) {
        do {
            // 跳过空白
            while (check(TokenType::T_WHITESPACE)) {
                consume();
            }
            args.push_back(parseExpression());
            // 跳过空白
            while (check(TokenType::T_WHITESPACE)) {
                consume();
            }
        } while (match(TokenType::T_COMMA));
    }

    consume(TokenType::T_RPAREN, "期望 ')'");

    return std::make_unique<CallExpr>(
        std::move(callee), std::move(args), line, column
    );
}

// 辅助方法
const Token& Parser::peek(int offset) const {
    size_t pos = currentPos + offset;
    if (pos >= tokens.size()) {
        static Token eofToken(TokenType::T_EOF, "", 0, 0, 0);
        return eofToken;
    }
    return tokens[pos];
}

const Token& Parser::current() {
    return peek(0);
}

const Token& Parser::consume() {
    if (isAtEnd()) {
        throw std::runtime_error("意外的文件结束");
    }
    return tokens[currentPos++];
}

const Token& Parser::consume(TokenType type, const std::string& expected) {
    if (check(type)) {
        return consume();
    }

    throw std::runtime_error("期望 " + expected + " 在 " + std::to_string(current().getLine()) + ":" + std::to_string(current().getColumn()));
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return current().getType() == type;
}

bool Parser::check(TokenType type, const std::string& value) {
    if (isAtEnd()) return false;
    return current().getType() == type && current().getValue() == value;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        consume();
        return true;
    }
    return false;
}

bool Parser::match(TokenType type, const std::string& value) {
    if (check(type, value)) {
        consume();
        return true;
    }
    return false;
}

void Parser::synchronize() {
    consume();  // 跳过当前错误token

    while (!isAtEnd()) {
        if (current().getType() == TokenType::T_SEMICOLON) {
            consume(); // 消费分号
            return;
        }

        if (SYNCHRONIZATION_TOKENS.find(current().getType()) !=
            SYNCHRONIZATION_TOKENS.end()) {
            return;
        }

        consume();
    }
}

bool Parser::isAtEnd() {
    return currentPos >= tokens.size() ||
           tokens[currentPos].getType() == TokenType::T_EOF;
}

bool Parser::isDeclaration() {
    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }
    return check(TokenType::T_FUNCTION) ||
           check(TokenType::T_DECLARE);
}

bool Parser::isStatement() {
    // 跳过空白
    while (check(TokenType::T_WHITESPACE)) {
        consume();
    }
    return check(TokenType::T_RETURN) ||
           check(TokenType::T_ECHO) ||
           check(TokenType::T_VARIABLE) ||
           check(TokenType::T_IDENTIFIER) ||
           check(TokenType::T_LBRACE) ||
           check(TokenType::T_IF) ||
           check(TokenType::T_FOR) ||
           check(TokenType::T_SEMICOLON) ||
           check(TokenType::T_SINGLE_QUOTE) ||
           check(TokenType::T_NUMBER);  // 数字也可以开始一个表达式语句
}

// 调试方法
void Parser::debug() {
    std::cout << "当前token: " << current().getValue() << std::endl;

    std::cout << "下一个token: " << peek(1).getValue() << std::endl;
}

// 解析变量声明（PHP风格：$var = expr;）
std::unique_ptr<ASTNode> Parser::parseVariableDecl() {
    auto token = consume();
    std::string name = token.getValue();
    if (!name.empty() && name[0] == '$') {
        name = name.substr(1);
    }

    // 创建变量节点
    auto varExpr = std::make_unique<VariableExpr>(name, token.getLine(), token.getColumn());

    // 检查是否是赋值
    if (match(TokenType::T_ASSIGN)) {
        auto valueExpr = parseExpression();
        
        // 创建赋值表达式：$var = value
        auto assignExpr = std::make_unique<BinaryExpr>(
            std::move(varExpr), "=", std::move(valueExpr),
            token.getLine(), token.getColumn()
        );
        
        consume(TokenType::T_SEMICOLON, "期望 ';' 在变量声明之后");
        return std::make_unique<ExpressionStmt>(std::move(assignExpr), token.getLine(), token.getColumn());
    }

    consume(TokenType::T_SEMICOLON, "期望 ';' 在变量声明之后");
    // 没有初始化的变量，赋值为null
    auto nullLiteral = std::make_unique<LiteralExpr>("null", "null", token.getLine(), token.getColumn());
    auto assignExpr = std::make_unique<BinaryExpr>(
        std::move(varExpr), "=", std::move(nullLiteral),
        token.getLine(), token.getColumn()
    );
    return std::make_unique<ExpressionStmt>(std::move(assignExpr), token.getLine(), token.getColumn());
}

// 解析表达式语句
std::unique_ptr<ASTNode> Parser::parseExpressionStmt() {
    auto expr = parseExpression();
    consume(TokenType::T_SEMICOLON, "期望 ';' 在表达式之后");
    return std::make_unique<ExpressionStmt>(std::move(expr), expr->getLine(), expr->getColumn());
}

} // namespace PHPForge
