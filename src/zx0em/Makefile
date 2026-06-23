# zx0em - Optimized asset blob packer for ZX0
# Makefile

CC      = gcc
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -std=c11 -I. -Izx0 -Isalvador/src -Isalvador/src/libdivsufsort/include
LDFLAGS = -lm -lpthread

# Sources
ZX0_SRC = zx0/compress.c zx0/optimize.c zx0/memory.c zx0/zx0_compress_wrapper.c
SALVADOR_SRC = salvador/src/expand.c salvador/src/matchfinder.c salvador/src/shrink.c \
               salvador/src/libdivsufsort/lib/divsufsort.c \
               salvador/src/libdivsufsort/lib/divsufsort_utils.c \
               salvador/src/libdivsufsort/lib/sssort.c \
               salvador/src/libdivsufsort/lib/trsort.c
PACK_SRC = zx0em_pack.c blob_builder.c emit_header.c png_decode.c similarity.c preprocess.c text_bpe.c ttf_strip.c
ALL_SRC = $(PACK_SRC) $(ZX0_SRC) $(SALVADOR_SRC)

# Binary folder and default target
BIN_DIR = binaries
TARGET  = $(BIN_DIR)/zx0em_pack

# Check if profile data exists in backup directory to compile with PGO optimization
PGO_USE_FLAG = $(shell test -d profiles_backup && echo "-fprofile-use -fprofile-correction -Wno-error=coverage-mismatch" || echo "")

.PHONY: all clean test test1 test2 test-optimal debug pgo-clean pgo-profile pgo-run pgo-save pgo-optimize pgo dist-clean dist-bins

# Default build: optimized with native architecture and LTO (and PGO if profiles exist)
all: CFLAGS += -Ofast -flto -march=native $(PGO_USE_FLAG)
all:
	@mkdir -p $(BIN_DIR)
	@if [ -d profiles_backup ] && [ ! -f $(BIN_DIR)/zx0em_pack-preprocess.gcda ]; then cp profiles_backup/*.gcda $(BIN_DIR)/; fi
	@$(MAKE) $(TARGET)
	@if [ -d profiles_backup ]; then $(MAKE) pgo-clean; fi

$(TARGET): $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(TARGET) (PGO status: $(if $(PGO_USE_FLAG),active,inactive))"

clean: pgo-clean dist-clean
	rm -rf $(BIN_DIR)
	rm -rf output_test1 output_test2 output_test3_advanced output_test_pgo

# --- PGO (Profile-Guided Optimization) Pipeline ---

pgo-clean:
	find . -maxdepth 1 -name "*.gcda" -delete
	find . -maxdepth 1 -name "*.gcno" -delete
	rm -f $(BIN_DIR)/*.gcda $(BIN_DIR)/*.gcno

pgo-profile: pgo-clean
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -Ofast -flto -march=native -fprofile-generate -o $(TARGET) $(ALL_SRC) $(LDFLAGS)
	@echo "Built instrumented binary for profiling."

pgo-run:
	@echo "Running instrumented binary for profiling..."
	./$(TARGET) mixed_assets2 output_test_pgo --stats
	rm -rf output_test_pgo
	@echo "Profiling run completed. Profile data (.gcda) generated."

pgo-save:
	mkdir -p profiles_backup
	cp $(BIN_DIR)/*.gcda profiles_backup/
	@echo "Saved PGO profiles to profiles_backup/ directory."

pgo-optimize: pgo-save
	@mkdir -p $(BIN_DIR)
	@cp profiles_backup/*.gcda $(BIN_DIR)/
	$(CC) $(CFLAGS) -Ofast -flto -march=native -fprofile-use -fprofile-correction -o $(TARGET) $(ALL_SRC) $(LDFLAGS)
	@$(MAKE) pgo-clean
	@echo "Built optimized binary using profile data."

# Full PGO pipeline
pgo:
	@$(MAKE) pgo-profile
	@$(MAKE) pgo-run
	@$(MAKE) pgo-optimize
	@echo "PGO optimized build successful."

# --- Microarchitecture Targets ---

dist-clean:
	rm -f $(BIN_DIR)/zx0em_pack_x86_64_v1 $(BIN_DIR)/zx0em_pack_x86_64_v2 $(BIN_DIR)/zx0em_pack_x86_64_v3 $(BIN_DIR)/zx0em_pack_x86_64_v4

dist-bins: dist-clean
	@mkdir -p $(BIN_DIR)
	@if [ -d profiles_backup ]; then cp profiles_backup/*.gcda $(BIN_DIR)/; fi
	$(CC) $(CFLAGS) -Ofast -flto -march=x86-64 $(PGO_USE_FLAG) -o $(BIN_DIR)/zx0em_pack $(ALL_SRC) $(LDFLAGS) && mv $(BIN_DIR)/zx0em_pack $(BIN_DIR)/zx0em_pack_x86_64_v1
	@if [ -d profiles_backup ]; then cp profiles_backup/*.gcda $(BIN_DIR)/; fi
	$(CC) $(CFLAGS) -Ofast -flto -march=x86-64-v2 $(PGO_USE_FLAG) -o $(BIN_DIR)/zx0em_pack $(ALL_SRC) $(LDFLAGS) && mv $(BIN_DIR)/zx0em_pack $(BIN_DIR)/zx0em_pack_x86_64_v2
	@if [ -d profiles_backup ]; then cp profiles_backup/*.gcda $(BIN_DIR)/; fi
	$(CC) $(CFLAGS) -Ofast -flto -march=x86-64-v3 $(PGO_USE_FLAG) -o $(BIN_DIR)/zx0em_pack $(ALL_SRC) $(LDFLAGS) && mv $(BIN_DIR)/zx0em_pack $(BIN_DIR)/zx0em_pack_x86_64_v3
	@if [ -d profiles_backup ]; then cp profiles_backup/*.gcda $(BIN_DIR)/; fi
	$(CC) $(CFLAGS) -Ofast -flto -march=x86-64-v4 $(PGO_USE_FLAG) -o $(BIN_DIR)/zx0em_pack $(ALL_SRC) $(LDFLAGS) && mv $(BIN_DIR)/zx0em_pack $(BIN_DIR)/zx0em_pack_x86_64_v4
	@$(MAKE) pgo-clean
	@echo "All microarchitecture binaries built in '$(BIN_DIR)/' (PGO status: $(if $(PGO_USE_FLAG),active,inactive))."

# --- Testing and Debugging ---

# Test with mixed_assets1 (CAD-style icons)
test1: $(TARGET)
	@echo "=== Test 1: mixed_assets1 (CAD icons) ==="
	./$(TARGET) mixed_assets1 output_test1 --verbose --stats

# Test with mixed_assets2 (kcalc icons + fonts + translations)
test2: $(TARGET)
	@echo "=== Test 2: mixed_assets2 (kcalc full set) ==="
	./$(TARGET) mixed_assets2 output_test2 --verbose --stats

# Run both tests
test: test1 test2

# Run optimal solver (takes longer)
test-optimal: $(TARGET)
	@echo "=== Test Optimal: mixed_assets2 (with Saukas optimal solver) ==="
	./$(TARGET) mixed_assets2 output_test2_optimal --verbose --stats --optimal

# Build with debug/sanitizer
debug: CFLAGS += -g -fsanitize=address -fsanitize=undefined
debug:
	@mkdir -p $(BIN_DIR)
	@$(MAKE) $(TARGET)
