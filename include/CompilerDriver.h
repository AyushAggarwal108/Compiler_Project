#pragma once
#include <string>
#include <vector>

// ============================================================
//  CompilerDriver.h — Orchestrates the full compilation pipeline
//  Lexer → Parser → Semantic → CodeGen → [Linker]
// ============================================================

struct CompilerOptions {
    bool showTokens  = false;
    bool showAST     = false;
    bool showIR      = false;
    bool showASM     = false;
    bool showSymbols = false;
    bool linkMode    = false;
    bool optimize    = true;   // constant folding + DCE by default
    bool verbose     = false;
    std::string outputDir = "output/";
};

class CompilerDriver {
public:
    explicit CompilerDriver(CompilerOptions opts);

    // Compile a single source file; returns true on success
    bool compileFile(const std::string& filepath);

    // Link multiple previously compiled object files
    bool linkFiles(const std::vector<std::string>& filepaths);

    // Returns overall exit code (0 = success)
    int exitCode() const { return exitCode_; }

private:
    CompilerOptions opts_;
    int             exitCode_ = 0;

    // Internal helper: reads a source file into a string
    std::string readFile(const std::string& path) const;

    // Write text to output dir
    void writeOutput(const std::string& filename, const std::string& content) const;
};
