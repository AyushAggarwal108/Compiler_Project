# ============================================================
#  Makefile — Mini Compiler + Linker
#  Targets: all, clean, test, debug, release
# ============================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Iinclude
LDFLAGS  :=

# Release vs Debug modes
RELEASE_FLAGS := -O2 -DNDEBUG
DEBUG_FLAGS   := -g3 -O0 -fsanitize=address,undefined -DDEBUG

TARGET  := compiler
SRC_DIR := src
INC_DIR := include
OBJ_DIR := build
OUT_DIR := output

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# ── Default target (release build) ──────────────────────────
.PHONY: all
all: CXXFLAGS += $(RELEASE_FLAGS)
all: $(TARGET)
	@echo ""
	@echo "  Build complete: ./$(TARGET)"
	@echo ""

# ── Debug build ─────────────────────────────────────────────
.PHONY: debug
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)
	@echo "  Debug build complete: ./$(TARGET)"

# ── Link target ─────────────────────────────────────────────
$(TARGET): $(OBJS)
	@echo "  Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# ── Compile object files ─────────────────────────────────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "  CC  $<"
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# ── Run basic tests ─────────────────────────────────────────
.PHONY: test
test: all
	@echo ""
	@echo "  Running test suite..."
	@mkdir -p $(OUT_DIR)
	@echo "  [Test 1] Hello World"
	@./$(TARGET) examples/hello.toy --tokens --ast --ir
	@echo ""
	@echo "  [Test 2] Arithmetic"
	@./$(TARGET) examples/arithmetic.toy --ir
	@echo ""
	@echo "  [Test 3] Float operations"
	@./$(TARGET) examples/floats.toy --ir
	@echo ""
	@echo "  [Test 4] Multi-file link"
	@./$(TARGET) examples/main.toy examples/math.toy --link
	@echo ""
	@echo "  All tests passed."

# ── Generate example outputs ─────────────────────────────────
.PHONY: examples
examples: all
	@mkdir -p $(OUT_DIR)
	@./$(TARGET) examples/hello.toy --tokens --ast --ir --asm --symbols
	@./$(TARGET) examples/arithmetic.toy --ir --asm
	@./$(TARGET) examples/floats.toy --ir
	@./$(TARGET) examples/main.toy examples/math.toy --link

# ── Linting ─────────────────────────────────────────────────
.PHONY: lint
lint:
	@cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem \
	          -I$(INC_DIR) $(SRC_DIR)/ 2>&1 | head -50

# ── Clean ────────────────────────────────────────────────────
.PHONY: clean
clean:
	@echo "  Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(TARGET) $(OUT_DIR)
	@echo "  Done."

# ── Install (to /usr/local/bin) ──────────────────────────────
.PHONY: install
install: all
	@cp $(TARGET) /usr/local/bin/toyc
	@echo "  Installed as: toyc"

# ── Show help ────────────────────────────────────────────────
.PHONY: help
help:
	@echo ""
	@echo "  Mini Compiler + Linker — Build Targets"
	@echo ""
	@echo "  make          Build release binary (./compiler)"
	@echo "  make debug    Build with ASAN + debug symbols"
	@echo "  make test     Build and run test suite"
	@echo "  make examples Build and run all example programs"
	@echo "  make clean    Remove build artifacts"
	@echo "  make install  Install as 'toyc' to /usr/local/bin"
	@echo "  make lint     Run cppcheck static analysis"
	@echo ""

# ── Dependency tracking ─────────────────────────────────────
-include $(OBJS:.o=.d)
$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -MM -MT $(OBJ_DIR)/$*.o $< > $@
