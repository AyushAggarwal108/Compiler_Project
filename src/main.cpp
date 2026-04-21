#include "../include/CompilerDriver.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

// ============================================================
//  main.cpp — CLI entry point
//  Usage:
//    ./compiler source.toy
//    ./compiler source.toy --tokens
//    ./compiler source.toy --ast
//    ./compiler source.toy --ir
//    ./compiler source.toy --asm
//    ./compiler source.toy --symbols
//    ./compiler main.toy math.toy utils.toy --link
//    ./compiler source.toy --no-opt
//    ./compiler source.toy --verbose
// ============================================================

static void printUsage(const char* prog) {
    std::cerr << "\nUsage:\n"
              << "  " << prog << " <file.toy> [options]\n"
              << "  " << prog << " <file1.toy> <file2.toy> ... --link\n\n"
              << "Options:\n"
              << "  --tokens     Print token stream\n"
              << "  --ast        Print abstract syntax tree\n"
              << "  --ir         Print three-address IR\n"
              << "  --asm        Print pseudo-assembly\n"
              << "  --symbols    Print symbol table\n"
              << "  --link       Link multiple .toy files\n"
              << "  --no-opt     Disable constant folding & DCE\n"
              << "  --verbose    Verbose output\n"
              << "  --help       Show this message\n\n"
              << "Examples:\n"
              << "  " << prog << " examples/hello.toy\n"
              << "  " << prog << " examples/hello.toy --tokens --ast --ir\n"
              << "  " << prog << " examples/main.toy examples/math.toy --link\n\n";
}

static bool hasFlag(const std::vector<std::string>& args, const std::string& flag) {
    return std::find(args.begin(), args.end(), flag) != args.end();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);

    if (hasFlag(args, "--help") || hasFlag(args, "-h")) {
        printUsage(argv[0]);
        return 0;
    }

    // Separate source files from flags
    std::vector<std::string> sourceFiles;
    for (auto& arg : args) {
        if (arg.size() >= 4 && arg.substr(arg.size() - 4) == ".toy") {
            sourceFiles.push_back(arg);
        }
    }

    if (sourceFiles.empty()) {
        std::cerr << "Error: no .toy source files specified.\n";
        printUsage(argv[0]);
        return 1;
    }

    CompilerOptions opts;
    opts.showTokens  = hasFlag(args, "--tokens");
    opts.showAST     = hasFlag(args, "--ast");
    opts.showIR      = hasFlag(args, "--ir");
    opts.showASM     = hasFlag(args, "--asm");
    opts.showSymbols = hasFlag(args, "--symbols");
    opts.linkMode    = hasFlag(args, "--link");
    opts.optimize    = !hasFlag(args, "--no-opt");
    opts.verbose     = hasFlag(args, "--verbose");

    CompilerDriver driver(opts);

    if (opts.linkMode) {
        driver.linkFiles(sourceFiles);
    } else {
        // Default: show IR for single-file unless flags say otherwise
        if (!opts.showTokens && !opts.showAST && !opts.showIR && !opts.showASM) {
            opts.showIR = true;
            opts.showASM = true;
        }
        // Reinitialize driver with updated opts
        CompilerDriver driver2(opts);
        driver2.compileFile(sourceFiles[0]);
        return driver2.exitCode();
    }

    return driver.exitCode();
}
