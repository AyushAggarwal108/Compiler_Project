#pragma once
#include "Token.h"
#include "AST.h"
#include <vector>
#include <memory>
#include <string>

// ============================================================
//  Parser.h — Recursive-descent parser
//  Consumes a token stream, produces an AST.
// ============================================================

class Parser {
public:
    Parser(std::vector<Token> tokens, std::string filename);

    // Entry point: parse entire program.
    std::unique_ptr<ProgramNode> parse();

    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::vector<Token> tokens_;
    std::string        filename_;
    std::size_t        pos_ = 0;
    std::vector<std::string> errors_;

    // Token stream navigation
    const Token& peek(int offset = 0) const;
    const Token& current()            const { return peek(0); }
    const Token& advance();
    bool         check(TokenType t)   const;
    bool         match(TokenType t);
    const Token& expect(TokenType t, const std::string& context);
    bool         isAtEnd()            const;

    // Grammar rules (each returns an AST node or nullptr on error)
    std::unique_ptr<ProgramNode> parseProgram();
    ASTNodePtr                   parseStatement();
    ASTNodePtr                   parseVarDecl(const std::string& typeName);
    ASTNodePtr                   parseAssignOrExprStmt();
    ASTNodePtr                   parsePrint();

    // Expression hierarchy (precedence climbing via recursive descent)
    ASTNodePtr parseExpression();
    ASTNodePtr parseComparison();
    ASTNodePtr parseAddSub();
    ASTNodePtr parseMulDiv();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePrimary();

    // Error recovery: skip to next semicolon / brace
    void synchronize();
    void emitError(const Token& tok, const std::string& msg);
};
