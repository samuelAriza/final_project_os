# Makefile for GSEA (Gestión Segura y Eficiente de Archivos)
# Universidad EAFIT - Sistemas Operativos

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -O2 -pthread
DEBUG_FLAGS = -g -DDEBUG -fsanitize=address -fsanitize=undefined
LDFLAGS = -pthread -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/file_manager.c \
          $(SRC_DIR)/compression/compression.c \
          $(SRC_DIR)/compression/lz77.c \
          $(SRC_DIR)/compression/huffman.c \
          $(SRC_DIR)/compression/rle.c \
          $(SRC_DIR)/encryption/aes.c \
          $(SRC_DIR)/encryption/chacha20.c \
          $(SRC_DIR)/concurrency/thread_pool.c \
          $(SRC_DIR)/utils/arg_parser.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Executable
TARGET = $(BIN_DIR)/gsea

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)/compression
	@mkdir -p $(BUILD_DIR)/encryption
	@mkdir -p $(BUILD_DIR)/concurrency
	@mkdir -p $(BUILD_DIR)/utils
	@mkdir -p $(BIN_DIR)

# Link executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all
	@echo "Debug build complete"

# Run tests
test: all
	@echo "Running tests..."
	@./$(TARGET) -h
	@echo "Creating test files..."
	@mkdir -p test_data
	@echo "Hello, World! This is a test file for GSEA compression and encryption." > test_data/test1.txt
	@echo "Another test file with different content. Testing compression ratios." > test_data/test2.txt
	@echo "Test compression with LZ77..."
	@./$(TARGET) -c --comp-alg lz77 -i test_data/test1.txt -o test_data/test1.lz77 -v
	@echo "Test decompression with LZ77..."
	@./$(TARGET) -d --comp-alg lz77 -i test_data/test1.lz77 -o test_data/test1_restored.txt -v
	@echo "Test compression with Huffman..."
	@./$(TARGET) -c --comp-alg huffman -i test_data/test1.txt -o test_data/test1.huff -v
	@echo "Test decompression with Huffman..."
	@./$(TARGET) -d --comp-alg huffman -i test_data/test1.huff -o test_data/test1_huff_restored.txt -v
	@echo "Test compression with RLE..."
	@./$(TARGET) -c --comp-alg rle -i test_data/test1.txt -o test_data/test1.rle -v
	@echo "Test decompression with RLE..."
	@./$(TARGET) -d --comp-alg rle -i test_data/test1.rle -o test_data/test1_rle_restored.txt -v
	@echo "Test encryption..."
	@./$(TARGET) -e --enc-alg aes128 -i test_data/test1.txt -o test_data/test1.enc -k "testkey123" -v
	@echo "Test decryption..."
	@./$(TARGET) -u --enc-alg aes128 -i test_data/test1.enc -o test_data/test1_decrypted.txt -k "testkey123" -v
	@echo "Test encryption with ChaCha20..."
	@./$(TARGET) -e --enc-alg chacha20 -i test_data/test1.txt -o test_data/test1.chacha.enc -k "testkey123" -v
	@echo "Test decryption with ChaCha20..."
	@./$(TARGET) -u --enc-alg chacha20 -i test_data/test1.chacha.enc -o test_data/test1_chacha_decrypted.txt -k "testkey123" -v
	@echo "Test combined operations..."
	@./$(TARGET) -ce --comp-alg lz77 --enc-alg aes128 -i test_data/test2.txt -o test_data/test2.comp.enc -k "secret" -v
	@./$(TARGET) -du --enc-alg aes128 --comp-alg lz77 -i test_data/test2.comp.enc -o test_data/test2_restored.txt -k "secret" -v
	@echo "Test combined operations with ChaCha20..."
	@./$(TARGET) -ce --comp-alg huffman --enc-alg chacha20 -i test_data/test2.txt -o test_data/test2.huff.chacha.enc -k "secret" -v
	@./$(TARGET) -du --enc-alg chacha20 --comp-alg huffman -i test_data/test2.huff.chacha.enc -o test_data/test2_chacha_restored.txt -k "secret" -v
	@echo "Comparing original and restored files..."
	@diff test_data/test1.txt test_data/test1_restored.txt && echo "✓ LZ77 compression test passed" || echo "✗ LZ77 compression test failed"
	@diff test_data/test1.txt test_data/test1_huff_restored.txt && echo "✓ Huffman compression test passed" || echo "✗ Huffman compression test failed"
	@diff test_data/test1.txt test_data/test1_rle_restored.txt && echo "✓ RLE compression test passed" || echo "✗ RLE compression test failed"
	@diff test_data/test1.txt test_data/test1_decrypted.txt && echo "✓ AES encryption test passed" || echo "✗ AES encryption test failed"
	@diff test_data/test1.txt test_data/test1_chacha_decrypted.txt && echo "✓ ChaCha20 encryption test passed" || echo "✗ ChaCha20 encryption test failed"
	@diff test_data/test2.txt test_data/test2_restored.txt && echo "✓ Combined operations (LZ77+AES) test passed" || echo "✗ Combined operations test failed"
	@diff test_data/test2.txt test_data/test2_chacha_restored.txt && echo "✓ Combined operations (Huffman+ChaCha20) test passed" || echo "✗ Combined operations test failed"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -rf test_data
	@echo "Clean complete"

# Install (copy to /usr/local/bin)
install: all
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling gsea from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/gsea
	@echo "Uninstall complete"

# Check for memory leaks with valgrind
valgrind: debug
	@echo "Running valgrind..."
	@mkdir -p test_data
	@echo "Test data for memory leak detection" > test_data/valgrind_test.txt
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		./$(TARGET) -c --comp-alg lz77 -i test_data/valgrind_test.txt -o test_data/valgrind_test.lz77

# Generate documentation with Doxygen (if available)
docs:
	@if command -v doxygen > /dev/null; then \
		echo "Generating documentation..."; \
		doxygen Doxyfile; \
		echo "Documentation generated in docs/html/"; \
	else \
		echo "Doxygen not found. Please install doxygen to generate documentation."; \
	fi

# Show help
help:
	@echo "GSEA Makefile targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug symbols and sanitizers"
	@echo "  test      - Run automated tests"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin (requires sudo)"
	@echo "  valgrind  - Check for memory leaks"
	@echo "  docs      - Generate documentation (requires doxygen)"
	@echo "  help      - Show this help message"

# Phony targets
.PHONY: all directories debug test clean install uninstall valgrind docs help