#ifndef SCOPE_H
#define SCOPE_H

#include "symbol.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

namespace PHPForge {

// 作用域类
class Scope {
public:
    Scope(const std::string& name, Scope* parent = nullptr)
        : name(name), parent(parent) {}
    
    // 添加符号
    bool addSymbol(std::shared_ptr<Symbol> symbol) {
        const std::string& name = symbol->getName();
        if (symbols.find(name) != symbols.end()) {
            return false; // 符号已存在
        }
        symbols[name] = symbol;
        return true;
    }
    
    // 查找符号（只在当前作用域）
    std::shared_ptr<Symbol> lookup(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // 查找符号（在当前和所有父作用域中）
    std::shared_ptr<Symbol> lookupRecursively(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return it->second;
        }
        
        if (parent) {
            return parent->lookupRecursively(name);
        }
        
        return nullptr;
    }
    
    // 获取父作用域
    Scope* getParent() const { return parent; }
    
    // 获取作用域名称
    const std::string& getName() const { return name; }
    
    // 获取所有符号
    const std::unordered_map<std::string, std::shared_ptr<Symbol>>& getSymbols() const {
        return symbols;
    }
    
    void dump(int indent = 0) const {
        std::cout << std::string(indent * 2, ' ') << "Scope: " << name << std::endl;
        for (const auto& pair : symbols) {
            pair.second->dump(indent + 1);
        }
    }

private:
    std::string name;
    Scope* parent;
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
};

// 符号表管理器
class SymbolTable {
public:
    SymbolTable() {
        // 创建全局作用域
        globalScope = std::make_unique<Scope>("global");
        currentScope = globalScope.get();
    }
    
    // 进入新作用域
    void enterScope(const std::string& name) {
        auto newScope = std::make_unique<Scope>(name, currentScope);
        scopes.push_back(std::move(newScope));
        currentScope = scopes.back().get();
    }
    
    // 退出当前作用域
    void exitScope() {
        if (scopes.size() > 0) {
            scopes.pop_back();
            if (scopes.empty()) {
                currentScope = globalScope.get();
            } else {
                currentScope = scopes.back().get();
            }
        }
    }
    
    // 获取当前作用域
    Scope* getCurrentScope() const { return currentScope; }
    
    // 获取全局作用域
    Scope* getGlobalScope() const { return globalScope.get(); }
    
    // 添加符号到当前作用域
    bool addSymbol(std::shared_ptr<Symbol> symbol) {
        return currentScope->addSymbol(symbol);
    }
    
    // 查找符号
    std::shared_ptr<Symbol> lookup(const std::string& name) const {
        return currentScope->lookupRecursively(name);
    }
    
    // 在指定作用域查找符号
    std::shared_ptr<Symbol> lookupInScope(Scope* scope, const std::string& name) const {
        return scope->lookupRecursively(name);
    }
    
    void dump() const {
        globalScope->dump();
        for (const auto& scope : scopes) {
            scope->dump();
        }
    }

private:
    std::unique_ptr<Scope> globalScope;
    std::vector<std::unique_ptr<Scope>> scopes;
    Scope* currentScope;
};

} // namespace PHPForge

#endif // SCOPE_H
