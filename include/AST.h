#pragma once
#include "Token.h"
#include <memory>
#include <string>
#include <vector>

// ============================================================
//  AST.h — Abstract Syntax Tree node hierarchy
//  Uses the Visitor pattern for clean stage separation.
// ============================================================

// Forward declarations for visitor
struct ProgramNode;
struct VarDeclNode;
struct AssignNode;
struct BinaryExprNode;
struct UnaryExprNode;
struct IntLiteralNode;
struct FloatLiteralNode;
struct IdentifierNode;
struct PrintNode;
struct BlockNode;

// ============================================================
//  Visitor interface
// ============================================================
struct ASTVisitor {
    virtual ~ASTVisitor() = default;
    virtual void visit(ProgramNode&)      = 0;
    virtual void visit(VarDeclNode&)      = 0;
    virtual void visit(AssignNode&)       = 0;
    virtual void visit(BinaryExprNode&)   = 0;
    virtual void visit(UnaryExprNode&)    = 0;
    virtual void visit(IntLiteralNode&)   = 0;
    virtual void visit(FloatLiteralNode&) = 0;
    virtual void visit(IdentifierNode&)   = 0;
    virtual void visit(PrintNode&)        = 0;
    virtual void visit(BlockNode&)        = 0;
};

// ============================================================
//  Base ASTNode
// ============================================================
struct ASTNode {
    int line = 0;
    int col  = 0;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& v) = 0;
    // Pretty-print for --ast flag
    virtual void print(int indent = 0) const = 0;
protected:
    static std::string pad(int indent) { return std::string(indent * 2, ' '); }
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// ============================================================
//  Expression nodes
// ============================================================

struct IntLiteralNode : ASTNode {
    long long value;
    explicit IntLiteralNode(long long v, int ln, int c) : value(v) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct FloatLiteralNode : ASTNode {
    double value;
    explicit FloatLiteralNode(double v, int ln, int c) : value(v) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct IdentifierNode : ASTNode {
    std::string name;
    explicit IdentifierNode(std::string n, int ln, int c) : name(std::move(n)) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct BinaryExprNode : ASTNode {
    TokenType   op;
    std::string opLex;
    ASTNodePtr  left;
    ASTNodePtr  right;
    BinaryExprNode(TokenType op_, std::string lex, ASTNodePtr l, ASTNodePtr r, int ln, int c)
        : op(op_), opLex(std::move(lex)), left(std::move(l)), right(std::move(r))
        { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct UnaryExprNode : ASTNode {
    TokenType  op;
    ASTNodePtr operand;
    UnaryExprNode(TokenType op_, ASTNodePtr operand_, int ln, int c)
        : op(op_), operand(std::move(operand_)) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

// ============================================================
//  Statement nodes
// ============================================================

struct VarDeclNode : ASTNode {
    std::string typeName;   // "int" | "float"
    std::string varName;
    ASTNodePtr  initializer; // may be nullptr (zero-init)
    VarDeclNode(std::string type, std::string name, ASTNodePtr init, int ln, int c)
        : typeName(std::move(type)), varName(std::move(name)), initializer(std::move(init))
        { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct AssignNode : ASTNode {
    std::string varName;
    ASTNodePtr  expr;
    AssignNode(std::string name, ASTNodePtr expr_, int ln, int c)
        : varName(std::move(name)), expr(std::move(expr_)) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct PrintNode : ASTNode {
    ASTNodePtr expr;
    explicit PrintNode(ASTNodePtr e, int ln, int c) : expr(std::move(e)) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

struct BlockNode : ASTNode {
    std::vector<ASTNodePtr> stmts;
    explicit BlockNode(int ln, int c) { line=ln; col=c; }
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};

// ============================================================
//  Root
// ============================================================
struct ProgramNode : ASTNode {
    std::string                filename;
    std::vector<ASTNodePtr>    stmts;
    explicit ProgramNode(std::string fn) : filename(std::move(fn)) {}
    void accept(ASTVisitor& v) override { v.visit(*this); }
    void print(int i) const override;
};
