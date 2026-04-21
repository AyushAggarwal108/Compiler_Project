#pragma once
#include "AST.h"
#include "Semantic.h"
#include <string>
#include <vector>
#include <sstream>

// ============================================================
//  CodeGen.h — IR + Pseudo-Assembly generation
//  Produces Three-Address Code (TAC) and register-based asm.
// ============================================================

// ============================================================
//  IRInstruction — one three-address code instruction
// ============================================================
struct IRInstruction {
    enum class Op {
        ASSIGN,      // dst = src
        ADD, SUB, MUL, DIV,
        NEG,
        PRINT,
        LABEL,
        NOP          // Dead code / placeholder
    };

    Op          op;
    std::string dst;
    std::string src1;
    std::string src2;
    bool        isDead = false;  // marked by dead-code elimination

    // Factory helpers
    static IRInstruction assign(std::string d, std::string s)
        { return {Op::ASSIGN, std::move(d), std::move(s), ""}; }
    static IRInstruction binop(Op o, std::string d, std::string s1, std::string s2)
        { return {o, std::move(d), std::move(s1), std::move(s2)}; }
    static IRInstruction print(std::string src)
        { return {Op::PRINT, "", std::move(src), ""}; }
    static IRInstruction nop()
        { IRInstruction i; i.op=Op::NOP; i.isDead=true; return i; }

    std::string toString() const;
};

// ============================================================
//  ObjectFile — output of one compilation unit
// ============================================================
struct ObjectFile {
    std::string               sourceFile;
    std::vector<IRInstruction> irCode;
    std::vector<std::string>   asmCode;
    std::vector<std::string>   exportedSymbols;  // globals defined here
    std::vector<std::string>   importedSymbols;  // globals referenced but not defined
};

// ============================================================
//  CodeGenerator — visits AST, emits IR & pseudo-assembly
// ============================================================
class CodeGenerator : public ASTVisitor {
public:
    explicit CodeGenerator(std::string filename, SymbolTable& symTab);

    // Main entry: returns an ObjectFile ready for the linker
    ObjectFile generate(ProgramNode& root);

    // Optimization passes
    void constantFolding();
    void deadCodeElimination();

    // Dump IR as text
    std::string irToString() const;
    // Dump pseudo-assembly as text
    std::string asmToString() const;

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
    std::string    filename_;
    SymbolTable&   symTab_;
    int            tempCounter_ = 0;

    std::vector<IRInstruction> ir_;
    std::vector<std::string>   asm_;
    std::string                exprResult_;  // result name propagated upward

    // Temp variable allocator
    std::string newTemp();

    // IR emitters
    void emitIR(IRInstruction instr);

    // Assembly emitters
    void emitASM(const std::string& line);
    void generateASMFromIR();

    // Collect symbol info
    std::vector<std::string> collectExports() const;
    std::vector<std::string> collectImports() const;
};
