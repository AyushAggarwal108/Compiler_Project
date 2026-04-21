#include "../include/Parser.h"
#include <stdexcept>
#include <sstream>

// ============================================================
//  Parser.cpp — Recursive-descent parser implementation
// ============================================================

Parser::Parser(std::vector<Token> tokens, std::string filename)
    : tokens_(std::move(tokens)), filename_(std::move(filename)) {}

// ──────────────────────────────────────────────────────────
//  Token navigation
// ──────────────────────────────────────────────────────────
const Token& Parser::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= tokens_.size()) return tokens_.back(); // EOF
    return tokens_[idx];
}

const Token& Parser::advance() {
    if (!isAtEnd()) ++pos_;
    return tokens_[pos_ - 1];
}

bool Parser::check(TokenType t) const {
    return !isAtEnd() && current().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType t, const std::string& context) {
    if (check(t)) return advance();
    emitError(current(), "Expected " + tokenTypeName(t) + " " + context +
              ", but got '" + current().lexeme + "'");
    // Return current (even though wrong) to allow partial recovery
    return current();
}

bool Parser::isAtEnd() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_FILE;
}

// ──────────────────────────────────────────────────────────
//  Entry point
// ──────────────────────────────────────────────────────────
std::unique_ptr<ProgramNode> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto prog = std::make_unique<ProgramNode>(filename_);
    while (!isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) prog->stmts.push_back(std::move(stmt));
    }
    return prog;
}

// ──────────────────────────────────────────────────────────
//  Statements
// ──────────────────────────────────────────────────────────
ASTNodePtr Parser::parseStatement() {
    // Type keywords → variable declaration
    if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT)) {
        std::string typeName = current().lexeme;
        advance();
        return parseVarDecl(typeName);
    }
    if (check(TokenType::KW_PRINT)) return parsePrint();

    // Otherwise: assignment or bare expression statement
    auto stmt = parseAssignOrExprStmt();
    return stmt;
}

ASTNodePtr Parser::parseVarDecl(const std::string& typeName) {
    // Expect: IDENTIFIER [= expr] ;
    int ln = current().line, cn = current().col;

    if (!check(TokenType::IDENTIFIER)) {
        emitError(current(), "Expected variable name after type '" + typeName + "'");
        synchronize();
        return nullptr;
    }
    std::string varName = current().lexeme;
    advance();

    ASTNodePtr init;
    if (match(TokenType::OP_ASSIGN)) {
        init = parseExpression();
    }

    expect(TokenType::SEMICOLON, "after variable declaration");
    return std::make_unique<VarDeclNode>(typeName, varName, std::move(init), ln, cn);
}

ASTNodePtr Parser::parseAssignOrExprStmt() {
    // Lookahead: IDENTIFIER = expr ;  (assignment)
    if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::OP_ASSIGN) {
        int ln = current().line, cn = current().col;
        std::string varName = current().lexeme;
        advance(); // consume identifier
        advance(); // consume '='
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "after assignment");
        return std::make_unique<AssignNode>(varName, std::move(expr), ln, cn);
    }

    // Otherwise treat as expression statement (e.g. function call in future)
    int ln = current().line, cn = current().col;
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "after expression statement");
    // For now, wrap in a print node if it's useful — otherwise discard
    // (a bare expression has no effect in this language)
    (void)ln; (void)cn;
    return expr; // caller may ignore
}

ASTNodePtr Parser::parsePrint() {
    int ln = current().line, cn = current().col;
    advance(); // consume 'print'
    expect(TokenType::LPAREN, "after 'print'");
    auto expr = parseExpression();
    expect(TokenType::RPAREN, "after print expression");
    expect(TokenType::SEMICOLON, "after print statement");
    return std::make_unique<PrintNode>(std::move(expr), ln, cn);
}

// ──────────────────────────────────────────────────────────
//  Expressions (precedence: comparison < +/- < */÷ < unary < primary)
// ──────────────────────────────────────────────────────────
ASTNodePtr Parser::parseExpression() {
    return parseComparison();
}

ASTNodePtr Parser::parseComparison() {
    auto left = parseAddSub();

    while (check(TokenType::OP_EQ)  || check(TokenType::OP_NEQ) ||
           check(TokenType::OP_LT)  || check(TokenType::OP_GT)  ||
           check(TokenType::OP_LEQ) || check(TokenType::OP_GEQ)) {
        int ln = current().line, cn = current().col;
        TokenType op = current().type;
        std::string lex = current().lexeme;
        advance();
        auto right = parseAddSub();
        left = std::make_unique<BinaryExprNode>(op, lex, std::move(left), std::move(right), ln, cn);
    }
    return left;
}

ASTNodePtr Parser::parseAddSub() {
    auto left = parseMulDiv();

    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) {
        int ln = current().line, cn = current().col;
        TokenType op = current().type;
        std::string lex = current().lexeme;
        advance();
        auto right = parseMulDiv();
        left = std::make_unique<BinaryExprNode>(op, lex, std::move(left), std::move(right), ln, cn);
    }
    return left;
}

ASTNodePtr Parser::parseMulDiv() {
    auto left = parseUnary();

    while (check(TokenType::OP_STAR) || check(TokenType::OP_SLASH)) {
        int ln = current().line, cn = current().col;
        TokenType op = current().type;
        std::string lex = current().lexeme;
        advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryExprNode>(op, lex, std::move(left), std::move(right), ln, cn);
    }
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (check(TokenType::OP_MINUS)) {
        int ln = current().line, cn = current().col;
        advance();
        auto operand = parsePrimary();
        return std::make_unique<UnaryExprNode>(TokenType::OP_MINUS, std::move(operand), ln, cn);
    }
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary() {
    int ln = current().line, cn = current().col;

    if (check(TokenType::INT_LITERAL)) {
        long long val = std::stoll(current().lexeme);
        advance();
        return std::make_unique<IntLiteralNode>(val, ln, cn);
    }

    if (check(TokenType::FLOAT_LITERAL)) {
        double val = std::stod(current().lexeme);
        advance();
        return std::make_unique<FloatLiteralNode>(val, ln, cn);
    }

    if (check(TokenType::IDENTIFIER)) {
        std::string name = current().lexeme;
        advance();
        return std::make_unique<IdentifierNode>(name, ln, cn);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "after grouped expression");
        return expr;
    }

    emitError(current(), "Unexpected token '" + current().lexeme + "' in expression");
    synchronize();
    return nullptr;
}

// ──────────────────────────────────────────────────────────
//  Error handling
// ──────────────────────────────────────────────────────────
void Parser::emitError(const Token& tok, const std::string& msg) {
    errors_.push_back(filename_ + ":" + std::to_string(tok.line) + ":" +
                      std::to_string(tok.col) + ": parse error: " + msg);
}

void Parser::synchronize() {
    // Panic-mode recovery: discard tokens until a statement boundary
    while (!isAtEnd()) {
        if (current().type == TokenType::SEMICOLON) { advance(); return; }
        switch (current().type) {
            case TokenType::KW_INT:
            case TokenType::KW_FLOAT:
            case TokenType::KW_PRINT:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_RETURN:
                return;
            default:
                advance();
        }
    }
}
