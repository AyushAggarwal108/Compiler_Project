#pragma once
#include "Token.h"
#include <string>
#include <vector>

// ============================================================
//  Lexer.h — Lexical analysis (tokenization)
//  Converts raw source text into a flat token stream.
// ============================================================

class Lexer {
public:
    explicit Lexer(std::string source, std::string filename = "<stdin>");

    // Main entry point: tokenize the entire source.
    std::vector<Token> tokenize();

    // Returns the list of lexer-level error messages.
    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::string source_;
    std::string filename_;
    std::size_t pos_  = 0;
    int         line_ = 1;
    int         col_  = 1;
    std::vector<std::string> errors_;

    // Character inspection helpers
    char peek(int offset = 0) const;
    char advance();
    bool isAtEnd() const;

    // Token builders
    Token makeToken(TokenType t, const std::string& lex) const;
    void  skipWhitespaceAndComments();

    // Specialised scanners
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanOperatorOrDelimiter();

    void  emitError(const std::string& msg);

    // Keyword lookup table
    static TokenType keywordType(const std::string& word);
};
