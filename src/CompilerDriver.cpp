#include "../include/CompilerDriver.h"
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Semantic.h"
#include "../include/CodeGen.h"
#include "../include/Linker.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

// ============================================================
//  CompilerDriver.cpp — Full pipeline orchestration
// ============================================================

// ──────────────────────────────────────────────────────────
//  ANSI color codes for terminal output
// ──────────────────────────────────────────────────────────
namespace Color {
    static const char* RESET   = "\033[0m";
    static const char* RED     = "\033[1;31m";
    static const char* GREEN   = "\033[1;32m";
    static const char* YELLOW  = "\033[1;33m";
    static const char* CYAN    = "\033[1;36m";
    static const char* BOLD    = "\033[1m";
    static const char* DIM     = "\033[2m";
    static const char* MAGENTA = "\033[1;35m";
    static const char* BLUE    = "\033[1;34m";
}

static void printSuccess(const std::string& msg) {
    std::cout << Color::GREEN << "  ✔ " << Color::RESET << msg << "\n";
}
static void printError(const std::string& msg) {
    std::cerr << Color::RED << "  ✘ " << Color::RESET << msg << "\n";
}
static void printWarning(const std::string& msg) {
    std::cout << Color::YELLOW << "  ⚠ " << Color::RESET << msg << "\n";
}
static void printSection(const std::string& title) {
    std::cout << "\n" << Color::CYAN << Color::BOLD
              << "══════ " << title << " ══════"
              << Color::RESET << "\n";
}
static void printBanner() {
    std::cout << Color::BLUE << Color::BOLD;
    std::cout << R"(
 ╔══════════════════════════════════════════════════════╗
 ║        Mini Compiler + Linker  [.toy language]       ║
 ║   Lexer → Parser → Semantic → CodeGen → Linker       ║
 ╚══════════════════════════════════════════════════════╝
)" << Color::RESET;
}

// ──────────────────────────────────────────────────────────
//  CompilerDriver
// ──────────────────────────────────────────────────────────
CompilerDriver::CompilerDriver(CompilerOptions opts) : opts_(std::move(opts)) {}

std::string CompilerDriver::readFile(const std::string& path) const {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void CompilerDriver::writeOutput(const std::string& filename, const std::string& content) const {
    namespace fs = std::filesystem;
    fs::create_directories(opts_.outputDir);
    std::string path = opts_.outputDir + filename;
    std::ofstream f(path);
    if (f.is_open()) {
        f << content;
        std::cout << Color::DIM << "    → Written to " << path << Color::RESET << "\n";
    }
}

bool CompilerDriver::compileFile(const std::string& filepath) {
    printBanner();

    auto start = std::chrono::high_resolution_clock::now();

    std::cout << Color::BOLD << "\n  Source: " << Color::RESET
              << Color::MAGENTA << filepath << Color::RESET << "\n";

    // ── Read source ──────────────────────────────────────
    std::string source;
    try {
        source = readFile(filepath);
    } catch (std::exception& e) {
        printError(e.what());
        exitCode_ = 1;
        return false;
    }

    // ── Stage 1: Lexing ──────────────────────────────────
    printSection("Stage 1: Lexical Analysis");
    Lexer lexer(source, filepath);
    auto tokens = lexer.tokenize();

    if (lexer.hasErrors()) {
        for (auto& e : lexer.errors()) printError(e);
        exitCode_ = 1;
        return false;
    }
    printSuccess("Lexing successful  (" + std::to_string(tokens.size()) + " tokens)");

    if (opts_.showTokens) {
        printSection("Token Stream");
        for (auto& tok : tokens) {
            std::cout << "  " << Color::DIM << tok.toString() << Color::RESET << "\n";
        }
    }

    // ── Stage 2: Parsing ─────────────────────────────────
    printSection("Stage 2: Parsing");
    Parser parser(tokens, filepath);
    auto ast = parser.parse();

    if (parser.hasErrors()) {
        for (auto& e : parser.errors()) printError(e);
        exitCode_ = 1;
        return false;
    }
    printSuccess("Parsing successful");

    if (opts_.showAST) {
        printSection("Abstract Syntax Tree");
        ast->print(0);
    }

    // ── Stage 3: Semantic Analysis ───────────────────────
    printSection("Stage 3: Semantic Analysis");
    SemanticAnalyzer sema(filepath);
    bool semOk = sema.analyze(*ast);

    for (auto& w : sema.warnings()) printWarning(w);

    if (!semOk) {
        for (auto& e : sema.errors()) printError(e);
        exitCode_ = 1;
        return false;
    }
    printSuccess("Semantic checks passed");

    if (opts_.showSymbols) {
        sema.symbolTable().dump();
    }

    // ── Stage 4: Code Generation ─────────────────────────
    printSection("Stage 4: Code Generation");
    // Re-analyze with fresh semantic context for codegen's symbol table
    SemanticAnalyzer sema2(filepath);
    sema2.analyze(*ast);

    CodeGenerator codegen(filepath, sema2.symbolTable());
    ObjectFile obj = codegen.generate(*ast);

    printSuccess("IR generated  (" + std::to_string(obj.irCode.size()) + " instructions)");
    if (opts_.optimize) {
        printSuccess("Optimizations applied (constant folding + DCE)");
    }
    printSuccess("Pseudo-assembly generated");

    if (opts_.showIR) {
        printSection("Intermediate Representation (Three-Address Code)");
        std::cout << codegen.irToString();
    }

    if (opts_.showASM) {
        printSection("Pseudo-Assembly Output");
        std::cout << codegen.asmToString();
    }

    // ── Write outputs ────────────────────────────────────
    namespace fs = std::filesystem;
    std::string base = fs::path(filepath).stem().string();

    printSection("Output Files");
    writeOutput(base + ".ir",  codegen.irToString());
    writeOutput(base + ".asm", codegen.asmToString());

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "\n" << Color::GREEN << Color::BOLD
              << "  ✔ Compilation successful  ["
              << std::fixed << std::setprecision(2) << ms << " ms]\n"
              << Color::RESET;

    return true;
}

bool CompilerDriver::linkFiles(const std::vector<std::string>& filepaths) {
    printBanner();
    printSection("Linker Mode  (" + std::to_string(filepaths.size()) + " units)");

    Linker linker;
    bool allOk = true;

    for (auto& fp : filepaths) {
        std::cout << Color::BOLD << "\n  Compiling: " << Color::MAGENTA << fp << Color::RESET << "\n";
        try {
            std::string source = readFile(fp);

            Lexer  lexer(source, fp);
            auto   tokens = lexer.tokenize();
            if (lexer.hasErrors()) {
                for (auto& e : lexer.errors()) printError(e);
                allOk = false; continue;
            }

            Parser parser(tokens, fp);
            auto   ast = parser.parse();
            if (parser.hasErrors()) {
                for (auto& e : parser.errors()) printError(e);
                allOk = false; continue;
            }

            SemanticAnalyzer sema(fp);
            if (!sema.analyze(*ast)) {
                for (auto& e : sema.errors()) printError(e);
                allOk = false; continue;
            }
            for (auto& w : sema.warnings()) printWarning(w);

            SemanticAnalyzer sema2(fp);
            sema2.analyze(*ast);
            CodeGenerator codegen(fp, sema2.symbolTable());
            ObjectFile obj = codegen.generate(*ast);
            printSuccess("Compiled: " + fp);
            linker.addObject(std::move(obj));

        } catch (std::exception& e) {
            printError(e.what());
            allOk = false;
        }
    }

    if (!allOk) {
        printError("Link aborted due to compilation errors");
        exitCode_ = 1;
        return false;
    }

    printSection("Linking");
    LinkedOutput result = linker.link();

    for (auto& w : linker.warnings()) printWarning(w);
    if (!result.success) {
        for (auto& e : linker.errors()) printError(e);
        exitCode_ = 1;
        return false;
    }

    // Dump symbol map
    std::cout << result.symbolMap;

    // Build merged IR text
    std::ostringstream irOut;
    irOut << "; ============================================================\n";
    irOut << "; Linked Output IR\n";
    irOut << "; ============================================================\n";
    for (std::size_t i = 0; i < result.mergedIR.size(); ++i) {
        irOut << std::setw(4) << i << "  " << result.mergedIR[i].toString() << "\n";
    }

    // Build merged ASM text
    std::ostringstream asmOut;
    for (auto& line : result.mergedASM) asmOut << line << "\n";

    printSection("Output Files");
    writeOutput("linked.ir",  irOut.str());
    writeOutput("linked.asm", asmOut.str());
    writeOutput("linked.sym", result.symbolMap);

    printSuccess("Linked executable generated");
    std::cout << "\n" << Color::GREEN << Color::BOLD
              << "  ✔ Link successful\n" << Color::RESET;
    return true;
}
