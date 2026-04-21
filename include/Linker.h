#pragma once
#include "CodeGen.h"
#include <string>
#include <vector>
#include <unordered_map>

// ============================================================
//  Linker.h — Symbol resolution across multiple object files
//  Detects duplicate symbols, undefined references,
//  and produces a final merged "executable" text.
// ============================================================

struct LinkedOutput {
    std::vector<IRInstruction>  mergedIR;
    std::vector<std::string>    mergedASM;
    std::string                 symbolMap;   // human-readable symbol table
    bool                        success = false;
};

class Linker {
public:
    Linker() = default;

    // Add a compiled object file to the link pool
    void addObject(ObjectFile obj);

    // Perform full link: resolve symbols, detect errors
    LinkedOutput link();

    const std::vector<std::string>& errors()   const { return errors_;   }
    const std::vector<std::string>& warnings() const { return warnings_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::vector<ObjectFile> objects_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    // Global symbol index: name -> defined-in-which-object
    struct SymbolEntry {
        std::string definedIn;
        int         count = 0;   // >1 means duplicate
    };
    std::unordered_map<std::string, SymbolEntry> globalSymbols_;

    void buildGlobalSymbolTable();
    void checkUndefinedReferences();
    void checkDuplicateSymbols();
    std::string buildSymbolMap() const;
};
