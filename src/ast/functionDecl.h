#ifndef FUNCTION_DECL_H
#define FUNCTION_DECL_H

#include "ASTNode.h"
#include <string>
#include <vector>
#include <memory>

namespace PHPForge {

// 前置声明
class BlockStmt;

    // 参数节点
    class Parameter : public ASTNode
    {
    public:
        Parameter(const std::string &name, const std::string &typeHint, int line, int column) : ASTNode(ASTNodeType::PARAMETER, line, column), name(name), typeHint(typeHint) {}

        const std::string getName() const { return name; }
        const std::string getTypeHint() const { return typeHint; }
        const std::string getType() const { return typeHint; }

        void dump(int indent = 0) const override;
        std::string toString() const override;

    private:
        std::string name;
        std::string typeHint;
    };

    // 函数声明节点
    class FunctionDecl : public ASTNode
    {
    public:
        FunctionDecl(const std::string &name,
                     const std::string &returnType,
                     std::vector<std::unique_ptr<Parameter>> params,
                     std::unique_ptr<BlockStmt> body,
                     int line, int column) : ASTNode(ASTNodeType::FUNCTION_DECL, line, column),
                                             name(name), returnType(returnType),
                                             params(std::move(params)), body(std::move(body)) {}
        const std::string& getName() const { return name; }
        const std::string& getReturnType() const { return returnType; }
        const std::vector<std::unique_ptr<Parameter>>& getParams() const { return params; }
        const std::vector<std::unique_ptr<Parameter>>& getParameters() const { return params; }
        const BlockStmt* getBody() const { return body.get(); }

        void dump(int indent = 0) const override;
        std::string toString() const override;

    private:
        std::string name;
        std::string returnType;
        std::vector<std::unique_ptr<Parameter>> params;
        std::unique_ptr<BlockStmt> body;
    };

} // namespace PHPForge

#endif // FUNCTION_DECL_H