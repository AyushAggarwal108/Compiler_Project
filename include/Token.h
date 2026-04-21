#pragma once
#include <string>
#include <unordered_map>

// ============================================================
//  Token.h — Token types and Token data structure
//  Mini Compiler + Linker | Systems Programming Project
// ============================================================

enum class TokenType {
    // Literals
    INT_LITERAL,
    FLOAT_LITERAL,
    IDENTIFIER,

    // Keywords
    KW_INT,
    KW_FLOAT,
    KW_PRINT,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_RETURN,

    // Operators
    OP_PLUS,
    OP_MINUS,
    OP_STAR,
    OP_SLASH,
    OP_ASSIGN,
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LEQ,
    OP_GEQ,

    // Delimiters
    SEMICOLON,
    COMMA,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,

    // Special
    END_OF_FILE,
    UNKNOWN
};

// Human-readable names for token types (used in error messages & debug output)
inline std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::INT_LITERAL:   return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::IDENTIFIER:    return "IDENTIFIER";
        case TokenType::KW_INT:        return "KW_INT";
        case TokenType::KW_FLOAT:      return "KW_FLOAT";
        case TokenType::KW_PRINT:      return "KW_PRINT";
        case TokenType::KW_IF:         return "KW_IF";
        case TokenType::KW_ELSE:       return "KW_ELSE";
        case TokenType::KW_WHILE:      return "KW_WHILE";
        case TokenType::KW_RETURN:     return "KW_RETURN";
        case TokenType::OP_PLUS:       return "OP_PLUS";
        case TokenType::OP_MINUS:      return "OP_MINUS";
        case TokenType::OP_STAR:       return "OP_STAR";
        case TokenType::OP_SLASH:      return "OP_SLASH";
        case TokenType::OP_ASSIGN:     return "OP_ASSIGN";
        case TokenType::OP_EQ:         return "OP_EQ";
        case TokenType::OP_NEQ:        return "OP_NEQ";
        case TokenType::OP_LT:         return "OP_LT";
        case TokenType::OP_GT:         return "OP_GT";
        case TokenType::OP_LEQ:        return "OP_LEQ";
        case TokenType::OP_GEQ:        return "OP_GEQ";
        case TokenType::SEMICOLON:     return "SEMICOLON";
        case TokenType::COMMA:         return "COMMA";
        case TokenType::LPAREN:        return "LPAREN";
        case TokenType::RPAREN:        return "RPAREN";
        case TokenType::LBRACE:        return "LBRACE";
        case TokenType::RBRACE:        return "RBRACE";
        case TokenType::END_OF_FILE:   return "EOF";
        default:                       return "UNKNOWN";
    }
}

// ============================================================
//  Token — Carries type, raw lexeme, and source location
// ============================================================
struct Token {
    TokenType   type;
    std::string lexeme;   // Raw text from source
    int         line;
    int         col;

    Token(TokenType t, std::string lex, int ln, int c)
        : type(t), lexeme(std::move(lex)), line(ln), col(c) {}

    bool is(TokenType t)    const { return type == t; }
    bool isNot(TokenType t) const { return type != t; }

    std::string toString() const {
        return "[" + tokenTypeName(type) + " | \"" + lexeme +
               "\" | L" + std::to_string(line) + ":C" + std::to_string(col) + "]";
    }
};
