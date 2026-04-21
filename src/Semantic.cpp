#include "../include/Semantic.h"
#include <iostream>
#include <iomanip>
#include <sstream>

// ============================================================
//  Semantic.cpp — Type checking, scoping, symbol resolution
// ============================================================

// ──────────────────────────────────────────────────────────
//  SymbolTable
// ──────────────────────────────────────────────────────────
void SymbolTable::enterScope() {
    scopes_.push_back({});
}

void SymbolTable::exitScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

bool SymbolTable::declare(const Symbol& sym) {
    if (scopes_.empty()) return false;
    auto& frame = scopes_.back();
    if (frame.count(sym.name)) return false; // Already declared in this scope
    frame[sym.name] = sym;
    return true;
}

std::optional<Symbol*> SymbolTable::lookup(const std::string& name) {
    // Search from innermost scope outward
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) return &it->second;
    }
    return std::nullopt;
}

void SymbolTable::markInitialized(const std::string& name) {
    auto sym = lookup(name);
    if (sym) (*sym)->isInitialized = true;
}

void SymbolTable::dump() const {
    std::cout << "\n┌─────────────────────────────────────────────────────────┐\n";
    std::cout <<   "│                     SYMBOL TABLE                       │\n";
    std::cout <<   "├──────────────┬───────┬───────┬──────┬─────────────────┤\n";
    std::cout <<   "│ Name         │ Type  │ Scope │ Slot │ Initialized     │\n";
    std::cout <<   "├──────────────┼───────┼───────┼──────┼─────────────────┤\n";

    for (std::size_t s = 0; s < scopes_.size(); ++s) {
        for (auto& [name, sym] : scopes_[s]) {
            std::cout << "│ " << std::left << std::setw(12) << name
                      << " │ " << std::setw(5) << sym.type
                      << " │ " << std::setw(5) << sym.scope
                      << " │ " << std::setw(4) << sym.slot
                      << " │ " << std::setw(15) << (sym.isInitialized ? "yes" : "no")
                      << " │\n";
        }
    }
    std::cout << "└──────────────┴───────┴───────┴──────┴─────────────────┘\n";
}

// ──────────────────────────────────────────────────────────
//  SemanticAnalyzer
// ──────────────────────────────────────────────────────────
SemanticAnalyzer::SemanticAnalyzer(std::string filename)
    : filename_(std::move(filename)) {}

bool SemanticAnalyzer::analyze(ProgramNode& root) {
    symTable_.enterScope(); // Global scope
    visit(root);
    // Note: dump() is called before exitScope() so symbols are visible
    return !hasErrors();
}

void SemanticAnalyzer::emitError(int line, int col, const std::string& msg) {
    errors_.push_back(filename_ + ":" + std::to_string(line) + ":" +
                      std::to_string(col) + ": semantic error: " + msg);
}

void SemanticAnalyzer::emitWarning(int line, int col, const std::string& msg) {
    warnings_.push_back(filename_ + ":" + std::to_string(line) + ":" +
                        std::to_string(col) + ": warning: " + msg);
}

bool SemanticAnalyzer::typesCompatible(const std::string& lhs, const std::string& rhs) const {
    if (lhs == rhs) return true;
    // Allow int → float implicit promotion
    if (lhs == "float" && rhs == "int") return true;
    return false;
}

std::string SemanticAnalyzer::promoteType(const std::string& a, const std::string& b) const {
    if (a == "float" || b == "float") return "float";
    return "int";
}

// ──────────────────────────────────────────────────────────
//  Visitor implementations
// ──────────────────────────────────────────────────────────
void SemanticAnalyzer::visit(ProgramNode& node) {
    for (auto& stmt : node.stmts) {
        if (stmt) stmt->accept(*this);
    }
}

void SemanticAnalyzer::visit(VarDeclNode& node) {
    // Check for redeclaration in current scope
    auto existing = symTable_.lookup(node.varName);
    if (existing && (*existing)->scope == symTable_.currentScope()) {
        emitError(node.line, node.col,
                  "Redeclaration of variable '" + node.varName + "' (first declared at line " +
                  std::to_string((*existing)->declLine) + ")");
        return;
    }

    std::string initType;
    if (node.initializer) {
        node.initializer->accept(*this);
        initType = currentExprType_;

        if (!typesCompatible(node.typeName, initType)) {
            emitError(node.line, node.col,
                      "Type mismatch: cannot initialize '" + node.typeName +
                      "' variable with '" + initType + "' value");
        }
        // Warn on narrowing float → int
        if (node.typeName == "int" && initType == "float") {
            emitWarning(node.line, node.col,
                        "Implicit narrowing conversion from 'float' to 'int' for '" + node.varName + "'");
        }
    }

    Symbol sym;
    sym.name          = node.varName;
    sym.type          = node.typeName;
    sym.scope         = symTable_.currentScope();
    sym.slot          = slotCounter_++;
    sym.declLine      = node.line;
    sym.isInitialized = (node.initializer != nullptr);
    symTable_.declare(sym);
}

void SemanticAnalyzer::visit(AssignNode& node) {
    auto sym = symTable_.lookup(node.varName);
    if (!sym) {
        emitError(node.line, node.col,
                  "Assignment to undeclared variable '" + node.varName + "'");
        return;
    }

    node.expr->accept(*this);
    std::string rhsType = currentExprType_;

    if (!typesCompatible((*sym)->type, rhsType)) {
        emitError(node.line, node.col,
                  "Type mismatch: cannot assign '" + rhsType +
                  "' to '" + (*sym)->type + "' variable '" + node.varName + "'");
    }

    symTable_.markInitialized(node.varName);
}

void SemanticAnalyzer::visit(BinaryExprNode& node) {
    if (node.left)  node.left->accept(*this);
    std::string leftType = currentExprType_;

    if (node.right) node.right->accept(*this);
    std::string rightType = currentExprType_;

    // Static division-by-zero detection
    if (node.op == TokenType::OP_SLASH) {
        if (auto* lit = dynamic_cast<IntLiteralNode*>(node.right.get())) {
            if (lit->value == 0) {
                emitError(node.line, node.col, "Division by zero detected at compile time");
            }
        }
        if (auto* lit = dynamic_cast<FloatLiteralNode*>(node.right.get())) {
            if (lit->value == 0.0) {
                emitError(node.line, node.col, "Division by zero detected at compile time");
            }
        }
    }

    currentExprType_ = promoteType(leftType, rightType);
}

void SemanticAnalyzer::visit(UnaryExprNode& node) {
    if (node.operand) node.operand->accept(*this);
    // Unary minus preserves type
}

void SemanticAnalyzer::visit(IntLiteralNode&) {
    currentExprType_ = "int";
}

void SemanticAnalyzer::visit(FloatLiteralNode&) {
    currentExprType_ = "float";
}

void SemanticAnalyzer::visit(IdentifierNode& node) {
    auto sym = symTable_.lookup(node.name);
    if (!sym) {
        emitError(node.line, node.col,
                  "Use of undeclared variable '" + node.name + "'");
        currentExprType_ = "int"; // recovery
        return;
    }
    if (!(*sym)->isInitialized) {
        emitWarning(node.line, node.col,
                    "Variable '" + node.name + "' may be used before initialization");
    }
    currentExprType_ = (*sym)->type;
}

void SemanticAnalyzer::visit(PrintNode& node) {
    if (node.expr) node.expr->accept(*this);
    // print() accepts any type — no further checking needed
}

void SemanticAnalyzer::visit(BlockNode& node) {
    symTable_.enterScope();
    for (auto& stmt : node.stmts) {
        if (stmt) stmt->accept(*this);
    }
    symTable_.exitScope();
}
