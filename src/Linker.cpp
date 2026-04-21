#include "../include/Linker.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// ============================================================
//  Linker.cpp — Multi-unit symbol resolution and output merging
// ============================================================

void Linker::addObject(ObjectFile obj) {
    objects_.push_back(std::move(obj));
}

LinkedOutput Linker::link() {
    errors_.clear();
    warnings_.clear();
    globalSymbols_.clear();

    buildGlobalSymbolTable();
    checkDuplicateSymbols();
    checkUndefinedReferences();

    LinkedOutput out;
    out.success = errors_.empty();

    if (!out.success) return out;

    // Merge all IR and ASM in order
    for (auto& obj : objects_) {
        out.mergedIR.emplace_back(IRInstruction{
            IRInstruction::Op::LABEL,
            "; === Unit: " + obj.sourceFile + " ===", "", ""
        });
        for (auto& instr : obj.irCode) {
            out.mergedIR.push_back(instr);
        }
        out.mergedASM.push_back("; === Unit: " + obj.sourceFile + " ===");
        for (auto& line : obj.asmCode) {
            out.mergedASM.push_back(line);
        }
    }

    out.symbolMap = buildSymbolMap();
    return out;
}

void Linker::buildGlobalSymbolTable() {
    for (auto& obj : objects_) {
        for (auto& sym : obj.exportedSymbols) {
            auto& entry = globalSymbols_[sym];
            entry.count++;
            if (entry.count == 1) {
                entry.definedIn = obj.sourceFile;
            } else {
                entry.definedIn += ", " + obj.sourceFile;
            }
        }
    }
}

void Linker::checkDuplicateSymbols() {
    for (auto& [name, entry] : globalSymbols_) {
        if (entry.count > 1) {
            errors_.push_back("linker error: duplicate symbol '" + name +
                              "' defined in: " + entry.definedIn);
        }
    }
}

void Linker::checkUndefinedReferences() {
    for (auto& obj : objects_) {
        for (auto& sym : obj.importedSymbols) {
            if (globalSymbols_.find(sym) == globalSymbols_.end()) {
                errors_.push_back("linker error: undefined reference to '" + sym +
                                  "' (referenced in " + obj.sourceFile + ")");
            }
        }
    }
}

std::string Linker::buildSymbolMap() const {
    std::ostringstream oss;
    oss << "\n┌──────────────────────────────────────────────────────────────┐\n";
    oss <<   "│                   LINKER SYMBOL MAP                         │\n";
    oss <<   "├──────────────────────────┬───────────────────────────────────┤\n";
    oss <<   "│ Symbol                   │ Defined In                        │\n";
    oss <<   "├──────────────────────────┼───────────────────────────────────┤\n";
    for (auto& [name, entry] : globalSymbols_) {
        oss << "│ " << std::left << std::setw(24) << name
            << " │ " << std::setw(33) << entry.definedIn << " │\n";
    }
    oss << "└──────────────────────────┴───────────────────────────────────┘\n";
    return oss.str();
}
