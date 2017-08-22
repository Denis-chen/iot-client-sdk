include ../base.makefile

TMP_DIR = output/obj
BUILD_DIR = output/build
OUTPUT_LIB = $(BUILD_DIR)/libtest.a
OUTPUT_TEST = $(BUILD_DIR)/main

CPPFLAGS += -g -O0

LIB_SRC = $(call mku_add_src_dir, src)
TEST_SRC = $(call mku_add_src_dir, tests/main)

all: lib test

lib: $(OUTPUT_LIB)

test: $(OUTPUT_TEST)

$(call mku_add_static_lib, $(OUTPUT_LIB), $(LIB_SRC), $(TMP_DIR))

$(call mku_add_executable, $(OUTPUT_TEST), $(TEST_SRC), $(TMP_DIR), $(OUTPUT_LIB))

clean:
	rm -f -R output/**
