# The BEES native edge: one C archive, libsilica_bees_native.a (gap ledger §4), built from
# src/ into dangerous_exposure_source/lib/ where Silica's sidecar link_library expects it.
THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
CC ?= clang
ARCH ?= arm64
MACOS_MIN_VERSION ?= 26.0
CFLAGS := -std=c11 -Wall -Wextra -O2 -arch $(ARCH) -mmacosx-version-min=$(MACOS_MIN_VERSION) \
	-I$(THIS_DIR)dangerous_exposure_source/bees
BUILD := $(THIS_DIR)build
LIB := $(THIS_DIR)dangerous_exposure_source/lib
SRCS := $(wildcard $(THIS_DIR)src/*.c)
OBJS := $(patsubst $(THIS_DIR)src/%.c,$(BUILD)/%.o,$(SRCS))
ARCHIVE := $(LIB)/libsilica_bees_native.a

.PHONY: all clean
all: $(ARCHIVE)

$(BUILD)/%.o: $(THIS_DIR)src/%.c $(wildcard $(THIS_DIR)dangerous_exposure_source/bees/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCHIVE): $(OBJS)
	@mkdir -p $(LIB)
	ar rcs $@ $^
	@echo "[native] $(ARCHIVE)"

clean:
	rm -rf $(BUILD) $(ARCHIVE)
