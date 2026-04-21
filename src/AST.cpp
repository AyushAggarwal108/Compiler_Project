#include "../include/AST.h"
#include <iostream>
#include <string>

// ============================================================
//  AST.cpp — Pretty-print implementations for all AST nodes
//  Uses box-drawing characters for a professional tree view.
// ============================================================

static void printTree(const std::string& label, int indent, bool last = false) {
    std::string prefix;
    for (int i = 0; i < indent - 1; ++i) prefix += "│  ";
    if (indent > 0) prefix += (last ? "└─ " : "├─ ");
    std::cout << prefix << label << "\n";
}

// ──────────────────────────────────────────────────────────
//  Literals
// ──────────────────────────────────────────────────────────
void IntLiteralNode::print(int i) const {
    printTree("IntLiteral(" + std::to_string(value) + ")", i);
}

void FloatLiteralNode::print(int i) const {
    printTree("FloatLiteral(" + std::to_string(value) + ")", i);
}

void IdentifierNode::print(int i) const {
    printTree("Identifier(" + name + ")", i);
}

// ──────────────────────────────────────────────────────────
//  Expressions
// ──────────────────────────────────────────────────────────
void BinaryExprNode::print(int i) const {
    printTree("BinaryExpr(op='" + opLex + "')", i);
    left->print(i + 1);
    right->print(i + 1);
}

void UnaryExprNode::print(int i) const {
    printTree("UnaryExpr(op='-')", i);
    operand->print(i + 1);
}

// ──────────────────────────────────────────────────────────
//  Statements
// ──────────────────────────────────────────────────────────
void VarDeclNode::print(int i) const {
    printTree("VarDecl(type=" + typeName + ", name=" + varName + ")", i);
    if (initializer) initializer->print(i + 1);
}

void AssignNode::print(int i) const {
    printTree("Assign(target=" + varName + ")", i);
    expr->print(i + 1);
}

void PrintNode::print(int i) const {
    printTree("Print", i);
    expr->print(i + 1);
}

void BlockNode::print(int i) const {
    printTree("Block", i);
    for (std::size_t idx = 0; idx < stmts.size(); ++idx) {
        stmts[idx]->print(i + 1);
    }
}

// ──────────────────────────────────────────────────────────
//  Root
// ──────────────────────────────────────────────────────────
void ProgramNode::print(int i) const {
    std::cout << "Program [" << filename << "]\n";
    for (std::size_t idx = 0; idx < stmts.size(); ++idx) {
        stmts[idx]->print(i + 1);
    }
}
