#ifndef PROGRAM_H
#define PROGRAM_H

#include "ASTNode.h"
#include <vector>

namespace PHPForge {
class Program : public ASTNode {
public:
    Program(int line = 1, int column = 1) : ASTNode(ASTNodeType::PROGRAM, line, column) {}
    ~Program() = default;

    void addStatement(ASTNodePtr stmt) {
        statements.push_back(std::move(stmt));
    }

    const std::vector<ASTNodePtr>& getStatements() const {
        return statements;
    }

    void dump(int indent = 0) const override;
    std::string toString() const override;

private:
    std::vector<ASTNodePtr> statements;
};

} // namespace PHPForge

#endif // PROGRAM_H