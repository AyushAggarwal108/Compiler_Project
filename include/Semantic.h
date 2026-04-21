#pragma once
#include "AST.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

// ============================================================
//  Semantic.h — Semantic analysis & symbol table
//  Checks types, scopes, redeclarations, undefined references.
// ============================================================

// ============================================================
//  Symbol — one entry in the symbol table
// ============================================================
struct Symbol {
    std::string name;
    std::string type;        // "int" | "float"
    int         scope = 0;   // nesting depth
    int         slot  = -1;  // memory slot index assigned by codegen
    int         declLine = 0;
    bool        isInitialized = false;
};

// ============================================================
//  SymbolTable — scoped symbol storage
// ============================================================
class SymbolTable {
public:
    void enterScope();
    void exitScope();

    // Declare: returns false if already declared in current scope
    bool declare(const Symbol& sym);

    // Lookup across all scopes (innermost first)
    std::optional<Symbol*> lookup(const std::string& name);

    // Mark a variable as initialized
    void markInitialized(const std::string& name);

    // Pretty-print the symbol table
    void dump() const;

    int currentScope() const { return static_cast<int>(scopes_.size()) - 1; }

private:
    // Stack of scope frames; each frame maps name → Symbol
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
};

// ============================================================
//  SemanticAnalyzer — walks AST, populates & queries SymbolTable
// ============================================================
class SemanticAnalyzer : public ASTVisitor {
public:
    explicit SemanticAnalyzer(std::string filename);

    // Returns false if any errors were found
    bool analyze(ProgramNode& root);

    const std::vector<std::string>& errors()   const { return errors_;   }
    const std::vector<std::string>& warnings() const { return warnings_; }
    bool hasErrors() const { return !errors_.empty(); }

    SymbolTable& symbolTable() { return symTable_; }

    // ASTVisitor overrides
    void visit(ProgramNode&)      override;
    void visit(VarDeclNode&)      override;
    void visit(AssignNode&)       override;
    void visit(BinaryExprNode&)   override;
    void visit(UnaryExprNode&)    override;
    void visit(IntLiteralNode&)   override;
    void visit(FloatLiteralNode&) override;
    void visit(IdentifierNode&)   override;
    void visit(PrintNode&)        override;
    void visit(BlockNode&)        override;

private:
    std::string  filename_;
    SymbolTable  symTable_;
    std::string  currentExprType_; // propagated upward during expression visits
    int          slotCounter_ = 0;

    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    void emitError(int line, int col, const std::string& msg);
    void emitWarning(int line, int col, const std::string& msg);

    // Type compatibility helper
    bool typesCompatible(const std::string& lhs, const std::string& rhs) const;
    std::string promoteType(const std::string& a, const std::string& b) const;
};
