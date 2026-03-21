#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <memory>
#include "../ast/ASTNode.h"
#include "../lexer/tokenType.h"

namespace PHPForge {

// 符号类型
enum class SymbolKind {
    VARIABLE,
    FUNCTION,
    PARAMETER,
    CLASS,
    METHOD
};

// 符号基类
class Symbol {
public:
    Symbol(SymbolKind kind, const std::string& name, const std::string& type = "")
        : kind(kind), name(name), type(type) {}
    
    virtual ~Symbol() = default;
    
    SymbolKind getKind() const { return kind; }
    const std::string& getName() const { return name; }
    const std::string& getType() const { return type; }
    void setType(const std::string& t) { type = t; }
    
    virtual void dump(int indent = 0) const = 0;

protected:
    SymbolKind kind;
    std::string name;
    std::string type;
};

// 变量符号
class VariableSymbol : public Symbol {
public:
    VariableSymbol(const std::string& name, const std::string& type = "")
        : Symbol(SymbolKind::VARIABLE, name, type) {}
    
    void dump(int indent = 0) const override;
};

// 函数符号
class FunctionSymbol : public Symbol {
public:
    FunctionSymbol(const std::string& name, const std::string& returnType)
        : Symbol(SymbolKind::FUNCTION, name, returnType) {}
    
    void addParameter(const std::string& paramName, const std::string& paramType) {
        parameterTypes.push_back(paramType);
        parameterNames.push_back(paramName);
    }
    
    const std::vector<std::string>& getParameterTypes() const { return parameterTypes; }
    const std::vector<std::string>& getParameterNames() const { return parameterNames; }
    
    void dump(int indent = 0) const override;

private:
    std::vector<std::string> parameterTypes;
    std::vector<std::string> parameterNames;
};

// 参数符号
class ParameterSymbol : public Symbol {
public:
    ParameterSymbol(const std::string& name, const std::string& type)
        : Symbol(SymbolKind::PARAMETER, name, type) {}
    
    void dump(int indent = 0) const override;
};

} // namespace PHPForge

#endif // SYMBOL_H
