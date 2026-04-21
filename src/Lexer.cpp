#include "../include/Lexer.h"
#include <cctype>
#include <stdexcept>

// ============================================================
//  Lexer.cpp — Lexical analysis implementation
// ============================================================

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

// ──────────────────────────────────────────────────────────
//  Public interface
// ──────────────────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        char c = peek();

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
        } else if (std::isdigit(c)) {
            tokens.push_back(scanNumber());
        } else {
            Token tok = scanOperatorOrDelimiter();
            if (tok.type != TokenType::UNKNOWN) {
                tokens.push_back(tok);
            } else {
                emitError("Unexpected character '" + std::string(1, c) + "'");
                advance(); // consume and continue
            }
        }
    }

    tokens.emplace_back(TokenType::END_OF_FILE, "", line_, col_);
    return tokens;
}

// ──────────────────────────────────────────────────────────
//  Private helpers
// ──────────────────────────────────────────────────────────
char Lexer::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= source_.size()) return '\0';
    return source_[idx];
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

Token Lexer::makeToken(TokenType t, const std::string& lex) const {
    return Token(t, lex, line_, col_);
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (std::isspace(c)) {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // Line comment: skip until newline
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && peek(1) == '*') {
            // Block comment
            advance(); advance(); // consume '/*'
            while (!isAtEnd()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance(); // consume '*/'
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::scanIdentifierOrKeyword() {
    int startLine = line_, startCol = col_;
    std::string lexeme;
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        lexeme += advance();
    }
    TokenType kw = keywordType(lexeme);
    return Token(kw, lexeme, startLine, startCol);
}

Token Lexer::scanNumber() {
    int startLine = line_, startCol = col_;
    std::string lexeme;
    bool isFloat = false;

    while (!isAtEnd() && std::isdigit(peek())) {
        lexeme += advance();
    }

    // Optional decimal part
    if (!isAtEnd() && peek() == '.' && std::isdigit(peek(1))) {
        isFloat = true;
        lexeme += advance(); // consume '.'
        while (!isAtEnd() && std::isdigit(peek())) {
            lexeme += advance();
        }
    }

    // Optional exponent (e.g. 1.5e3)
    if (!isAtEnd() && (peek() == 'e' || peek() == 'E')) {
        isFloat = true;
        lexeme += advance();
        if (!isAtEnd() && (peek() == '+' || peek() == '-')) {
            lexeme += advance();
        }
        while (!isAtEnd() && std::isdigit(peek())) {
            lexeme += advance();
        }
    }

    TokenType t = isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL;
    return Token(t, lexeme, startLine, startCol);
}

Token Lexer::scanOperatorOrDelimiter() {
    int startLine = line_, startCol = col_;
    char c = advance();
    std::string lex(1, c);

    switch (c) {
        case '+': return Token(TokenType::OP_PLUS,   lex, startLine, startCol);
        case '-': return Token(TokenType::OP_MINUS,  lex, startLine, startCol);
        case '*': return Token(TokenType::OP_STAR,   lex, startLine, startCol);
        case '/': return Token(TokenType::OP_SLASH,  lex, startLine, startCol);
        case ';': return Token(TokenType::SEMICOLON, lex, startLine, startCol);
        case ',': return Token(TokenType::COMMA,     lex, startLine, startCol);
        case '(': return Token(TokenType::LPAREN,    lex, startLine, startCol);
        case ')': return Token(TokenType::RPAREN,    lex, startLine, startCol);
        case '{': return Token(TokenType::LBRACE,    lex, startLine, startCol);
        case '}': return Token(TokenType::RBRACE,    lex, startLine, startCol);

        case '=':
            if (!isAtEnd() && peek() == '=') {
                lex += advance();
                return Token(TokenType::OP_EQ, lex, startLine, startCol);
            }
            return Token(TokenType::OP_ASSIGN, lex, startLine, startCol);

        case '!':
            if (!isAtEnd() && peek() == '=') {
                lex += advance();
                return Token(TokenType::OP_NEQ, lex, startLine, startCol);
            }
            break;

        case '<':
            if (!isAtEnd() && peek() == '=') {
                lex += advance();
                return Token(TokenType::OP_LEQ, lex, startLine, startCol);
            }
            return Token(TokenType::OP_LT, lex, startLine, startCol);

        case '>':
            if (!isAtEnd() && peek() == '=') {
                lex += advance();
                return Token(TokenType::OP_GEQ, lex, startLine, startCol);
            }
            return Token(TokenType::OP_GT, lex, startLine, startCol);

        default: break;
    }

    return Token(TokenType::UNKNOWN, lex, startLine, startCol);
}

void Lexer::emitError(const std::string& msg) {
    errors_.push_back(filename_ + ":" + std::to_string(line_) + ":" +
                      std::to_string(col_) + ": lexer error: " + msg);
}

TokenType Lexer::keywordType(const std::string& word) {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"int",    TokenType::KW_INT},
        {"float",  TokenType::KW_FLOAT},
        {"print",  TokenType::KW_PRINT},
        {"if",     TokenType::KW_IF},
        {"else",   TokenType::KW_ELSE},
        {"while",  TokenType::KW_WHILE},
        {"return", TokenType::KW_RETURN},
    };
    auto it = kw.find(word);
    return it != kw.end() ? it->second : TokenType::IDENTIFIER;
}
