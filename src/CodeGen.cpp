#include "../include/CodeGen.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unordered_set>

// ============================================================
//  CodeGen.cpp — Three-address code + pseudo-assembly generation
//  Also applies constant folding and dead-code elimination.
// ============================================================

// ──────────────────────────────────────────────────────────
//  IRInstruction::toString
// ──────────────────────────────────────────────────────────
std::string IRInstruction::toString() const {
    if (isDead) return "; [dead] (eliminated)";
    switch (op) {
        case Op::ASSIGN: return dst + " = " + src1;
        case Op::ADD:    return dst + " = " + src1 + " + " + src2;
        case Op::SUB:    return dst + " = " + src1 + " - " + src2;
        case Op::MUL:    return dst + " = " + src1 + " * " + src2;
        case Op::DIV:    return dst + " = " + src1 + " / " + src2;
        case Op::NEG:    return dst + " = -" + src1;
        case Op::PRINT:  return "print " + src1;
        case Op::LABEL:  return dst + ":";
        case Op::NOP:    return "; nop";
        default:         return "; unknown";
    }
}

// ──────────────────────────────────────────────────────────
//  CodeGenerator
// ──────────────────────────────────────────────────────────
CodeGenerator::CodeGenerator(std::string filename, SymbolTable& symTab)
    : filename_(std::move(filename)), symTab_(symTab) {}

ObjectFile CodeGenerator::generate(ProgramNode& root) {
    root.accept(*this);

    // Apply optimizations
    constantFolding();
    deadCodeElimination();

    // Generate pseudo-assembly from (possibly optimized) IR
    generateASMFromIR();

    ObjectFile obj;
    obj.sourceFile       = filename_;
    obj.irCode           = ir_;
    obj.asmCode          = asm_;
    obj.exportedSymbols  = collectExports();
    obj.importedSymbols  = collectImports();
    return obj;
}

std::string CodeGenerator::newTemp() {
    return "t" + std::to_string(tempCounter_++);
}

void CodeGenerator::emitIR(IRInstruction instr) {
    ir_.push_back(std::move(instr));
}

void CodeGenerator::emitASM(const std::string& line) {
    asm_.push_back(line);
}

// ──────────────────────────────────────────────────────────
//  AST Visitor implementations
// ──────────────────────────────────────────────────────────
void CodeGenerator::visit(ProgramNode& node) {
    for (auto& stmt : node.stmts) {
        if (stmt) stmt->accept(*this);
    }
}

void CodeGenerator::visit(VarDeclNode& node) {
    if (node.initializer) {
        node.initializer->accept(*this);
        std::string src = exprResult_;
        emitIR(IRInstruction::assign(node.varName, src));
    } else {
        // Zero-initialize
        emitIR(IRInstruction::assign(node.varName, "0"));
    }
}

void CodeGenerator::visit(AssignNode& node) {
    node.expr->accept(*this);
    std::string src = exprResult_;
    emitIR(IRInstruction::assign(node.varName, src));
}

void CodeGenerator::visit(BinaryExprNode& node) {
    node.left->accept(*this);
    std::string lhs = exprResult_;

    node.right->accept(*this);
    std::string rhs = exprResult_;

    std::string dst = newTemp();

    IRInstruction::Op opCode;
    switch (node.op) {
        case TokenType::OP_PLUS:  opCode = IRInstruction::Op::ADD; break;
        case TokenType::OP_MINUS: opCode = IRInstruction::Op::SUB; break;
        case TokenType::OP_STAR:  opCode = IRInstruction::Op::MUL; break;
        case TokenType::OP_SLASH: opCode = IRInstruction::Op::DIV; break;
        default:                  opCode = IRInstruction::Op::ADD; break; // comparison fallthrough
    }

    emitIR(IRInstruction::binop(opCode, dst, lhs, rhs));
    exprResult_ = dst;
}

void CodeGenerator::visit(UnaryExprNode& node) {
    node.operand->accept(*this);
    std::string src = exprResult_;
    std::string dst = newTemp();
    emitIR({IRInstruction::Op::NEG, dst, src, ""});
    exprResult_ = dst;
}

void CodeGenerator::visit(IntLiteralNode& node) {
    exprResult_ = std::to_string(node.value);
}

void CodeGenerator::visit(FloatLiteralNode& node) {
    std::ostringstream oss;
    oss << node.value;
    exprResult_ = oss.str();
}

void CodeGenerator::visit(IdentifierNode& node) {
    exprResult_ = node.name;
}

void CodeGenerator::visit(PrintNode& node) {
    node.expr->accept(*this);
    emitIR(IRInstruction::print(exprResult_));
}

void CodeGenerator::visit(BlockNode& node) {
    for (auto& stmt : node.stmts) {
        if (stmt) stmt->accept(*this);
    }
}

// ──────────────────────────────────────────────────────────
//  Optimization: Constant Folding
//  Replaces binary ops on two numeric literals with their result.
// ──────────────────────────────────────────────────────────
static bool isNumericLiteral(const std::string& s, double& val) {
    try {
        std::size_t idx;
        val = std::stod(s, &idx);
        return idx == s.size();
    } catch (...) { return false; }
}

void CodeGenerator::constantFolding() {
    // Map from temp name → constant value (if folded)
    std::unordered_map<std::string, std::string> constMap;

    for (auto& instr : ir_) {
        if (instr.isDead) continue;

        // Resolve known constants in operands
        auto resolve = [&](const std::string& s) -> std::string {
            auto it = constMap.find(s);
            return it != constMap.end() ? it->second : s;
        };

        std::string r1 = resolve(instr.src1);
        std::string r2 = resolve(instr.src2);

        double v1, v2;
        bool lit1 = isNumericLiteral(r1, v1);
        bool lit2 = !r2.empty() && isNumericLiteral(r2, v2);

        if (instr.op == IRInstruction::Op::ASSIGN && lit1) {
            // Propagate constant through assignment chain
            constMap[instr.dst] = r1;
            instr.src1 = r1;
        } else if (lit1 && lit2) {
            // Fold the binary operation
            double result = 0;
            bool folded = true;
            switch (instr.op) {
                case IRInstruction::Op::ADD: result = v1 + v2; break;
                case IRInstruction::Op::SUB: result = v1 - v2; break;
                case IRInstruction::Op::MUL: result = v1 * v2; break;
                case IRInstruction::Op::DIV:
                    if (v2 != 0) result = v1 / v2;
                    else         folded  = false;
                    break;
                default: folded = false; break;
            }
            if (folded) {
                // Emit as integer string if result is whole
                std::string resultStr;
                if (result == static_cast<long long>(result)) {
                    resultStr = std::to_string(static_cast<long long>(result));
                } else {
                    std::ostringstream oss; oss << result;
                    resultStr = oss.str();
                }
                instr.op   = IRInstruction::Op::ASSIGN;
                instr.src1 = resultStr;
                instr.src2.clear();
                constMap[instr.dst] = resultStr;
            }
        } else {
            // Not foldable — update operands if we resolved them
            instr.src1 = r1;
            if (!instr.src2.empty()) instr.src2 = r2;
        }
    }
}

// ──────────────────────────────────────────────────────────
//  Optimization: Dead Code Elimination
//  Marks assignments whose destination is never read again.
// ──────────────────────────────────────────────────────────
void CodeGenerator::deadCodeElimination() {
    // Collect all variables/temps that are ever read
    std::unordered_set<std::string> used;
    for (auto& instr : ir_) {
        if (!instr.src1.empty()) used.insert(instr.src1);
        if (!instr.src2.empty()) used.insert(instr.src2);
        if (instr.op == IRInstruction::Op::PRINT) used.insert(instr.src1);
    }

    // Mark assignments to temporaries that are never subsequently used
    for (auto& instr : ir_) {
        if (instr.isDead) continue;
        if (instr.op == IRInstruction::Op::ASSIGN || 
            instr.op == IRInstruction::Op::ADD     ||
            instr.op == IRInstruction::Op::SUB     ||
            instr.op == IRInstruction::Op::MUL     ||
            instr.op == IRInstruction::Op::DIV     ||
            instr.op == IRInstruction::Op::NEG) {
            // Only eliminate pure temporaries (tN), not user-declared variables
            if (!instr.dst.empty() && instr.dst[0] == 't' &&
                used.find(instr.dst) == used.end()) {
                instr.isDead = true;
            }
        }
    }
}

// ──────────────────────────────────────────────────────────
//  Assembly generation from IR
// ──────────────────────────────────────────────────────────
void CodeGenerator::generateASMFromIR() {
    asm_.clear();
    asm_.push_back("; ============================================================");
    asm_.push_back("; Pseudo-Assembly for: " + filename_);
    asm_.push_back("; ============================================================");
    asm_.push_back("SECTION .text");
    asm_.push_back("");

    int regIdx = 0;
    std::unordered_map<std::string, std::string> varReg;

    auto getReg = [&](const std::string& var) -> std::string {
        auto it = varReg.find(var);
        if (it != varReg.end()) return it->second;
        std::string reg = "R" + std::to_string(regIdx++);
        varReg[var] = reg;
        return reg;
    };

    for (auto& instr : ir_) {
        if (instr.isDead) {
            asm_.push_back("    ; [DCE] eliminated: " + instr.toString());
            continue;
        }
        switch (instr.op) {
            case IRInstruction::Op::ASSIGN: {
                double v; bool isLit = isNumericLiteral(instr.src1, v);
                std::string dstReg = getReg(instr.dst);
                if (isLit) {
                    asm_.push_back("    MOV " + dstReg + ", " + instr.src1);
                } else {
                    std::string srcReg = getReg(instr.src1);
                    asm_.push_back("    MOV " + dstReg + ", " + srcReg);
                }
                asm_.push_back("    STORE " + instr.dst + ", " + dstReg);
                break;
            }
            case IRInstruction::Op::ADD:
            case IRInstruction::Op::SUB:
            case IRInstruction::Op::MUL:
            case IRInstruction::Op::DIV: {
                static const std::unordered_map<int,std::string> mnemonics = {
                    {(int)IRInstruction::Op::ADD, "ADD"},
                    {(int)IRInstruction::Op::SUB, "SUB"},
                    {(int)IRInstruction::Op::MUL, "MUL"},
                    {(int)IRInstruction::Op::DIV, "DIV"},
                };
                std::string mn = mnemonics.at((int)instr.op);
                double v; bool isLit1 = isNumericLiteral(instr.src1, v);
                double v2; bool isLit2 = isNumericLiteral(instr.src2, v2);

                std::string r1 = isLit1 ? instr.src1 : "R" + std::to_string(regIdx);
                if (!isLit1) {
                    r1 = getReg(instr.src1);
                    asm_.push_back("    LOAD " + r1 + ", " + instr.src1);
                }
                std::string dstReg = getReg(instr.dst);
                asm_.push_back("    MOV " + dstReg + ", " + r1);

                if (isLit2) {
                    asm_.push_back("    " + mn + " " + dstReg + ", " + instr.src2);
                } else {
                    std::string r2 = getReg(instr.src2);
                    asm_.push_back("    LOAD " + r2 + ", " + instr.src2);
                    asm_.push_back("    " + mn + " " + dstReg + ", " + r2);
                }
                asm_.push_back("    STORE " + instr.dst + ", " + dstReg);
                break;
            }
            case IRInstruction::Op::NEG: {
                std::string srcReg = getReg(instr.src1);
                std::string dstReg = getReg(instr.dst);
                asm_.push_back("    LOAD " + srcReg + ", " + instr.src1);
                asm_.push_back("    NEG  " + dstReg + ", " + srcReg);
                asm_.push_back("    STORE " + instr.dst + ", " + dstReg);
                break;
            }
            case IRInstruction::Op::PRINT: {
                double v; bool isLit = isNumericLiteral(instr.src1, v);
                if (!isLit) {
                    std::string reg = getReg(instr.src1);
                    asm_.push_back("    LOAD " + reg + ", " + instr.src1);
                    asm_.push_back("    PRINT " + reg);
                } else {
                    asm_.push_back("    PRINT " + instr.src1);
                }
                break;
            }
            default: break;
        }
    }
    asm_.push_back("");
    asm_.push_back("    HALT");
}

// ──────────────────────────────────────────────────────────
//  String dumps
// ──────────────────────────────────────────────────────────
std::string CodeGenerator::irToString() const {
    std::ostringstream oss;
    oss << "; ============================================================\n";
    oss << "; Three-Address IR for: " << filename_ << "\n";
    oss << "; ============================================================\n";
    for (std::size_t i = 0; i < ir_.size(); ++i) {
        oss << std::setw(4) << i << "  " << ir_[i].toString() << "\n";
    }
    return oss.str();
}

std::string CodeGenerator::asmToString() const {
    std::ostringstream oss;
    for (auto& line : asm_) oss << line << "\n";
    return oss.str();
}

// ──────────────────────────────────────────────────────────
//  Symbol classification helpers
// ──────────────────────────────────────────────────────────
std::vector<std::string> CodeGenerator::collectExports() const {
    // All user-declared variable names that appear as assignment destinations
    std::unordered_set<std::string> exports;
    for (auto& instr : ir_) {
        if (!instr.dst.empty() && (instr.dst[0] != 't')) {
            exports.insert(instr.dst);
        }
    }
    return {exports.begin(), exports.end()};
}

std::vector<std::string> CodeGenerator::collectImports() const {
    // Variables referenced in src but not defined as dst in this unit
    std::unordered_set<std::string> defined, referenced;
    double v;
    for (auto& instr : ir_) {
        if (!instr.dst.empty() && instr.dst[0] != 't') defined.insert(instr.dst);
        if (!instr.src1.empty() && !isNumericLiteral(instr.src1, v) && instr.src1[0] != 't')
            referenced.insert(instr.src1);
        if (!instr.src2.empty() && !isNumericLiteral(instr.src2, v) && instr.src2[0] != 't')
            referenced.insert(instr.src2);
    }
    std::vector<std::string> imports;
    for (auto& r : referenced) {
        if (!defined.count(r)) imports.push_back(r);
    }
    return imports;
}
