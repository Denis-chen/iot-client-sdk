include lib/make_utils/base.makefile

# Build/Temp and Architecture dirs
ARCH = default
TMP_DIR = output/tmp/$(ARCH)
BUILD_DIR = output/build/$(ARCH)

# Library output file
OUTPUT_LIB_NAME = iotclient
OUTPUT_LIB = $(BUILD_DIR)/lib$(OUTPUT_LIB_NAME).a

# Test (executable) output file
OUTPUT_TEST = $(BUILD_DIR)/iot_client

# Library paths
LIB_CAJUN = lib/cajun-2.0.2
LIB_FMT = lib/fmt-3.0.1
LIB_NETLIB = lib/netlib
LIB_MBEDTLS = $(LIB_NETLIB)/lib/mbedtls-2.4.2
LIB_HTTP_PARSER = $(LIB_NETLIB)/lib/http-parser-2.7.1
LIB_CRYPTO = lib/milagro-crypto-c
LIB_PAHO = lib/paho.mqtt.embedded-c-master

# Includes
INCLUDE_DIRS = \
	-Iinclude \
	-I$(LIB_CAJUN) \
	-I$(LIB_FMT) \
	-I$(LIB_NETLIB)/include \
	-I$(LIB_MBEDTLS)/include \
	-I$(LIB_HTTP_PARSER) \
	-I$(LIB_CRYPTO)/include \
	-I$(LIB_PAHO)/MQTTClient/src -I$(LIB_PAHO)/MQTTPacket/src

# Uncomment following two lines to enable unused symbols stripping
#CPPSTRIP = -fdata-sections -ffunction-sections
#LDSTRIP = -Wl,--gc-sections

# C and C++ flags
CPPFLAGS += $(ARCH_CPPFLAGS) -g -O0 $(INCLUDE_DIRS) -DFMT_HEADER_ONLY -DC99 $(CPPSTRIP)
CXXFLAGS += -std=c++98
#CFLAGS += -std=c99

# Additional library search paths
LIB_DIRS =

# Additional libraries
LDLIBS =

# Uncomment the following line to enable executable static linking against system libraries
#ARCH_LDFLAGS = -static -static-libgcc -static-libstdc++

# Linker flags
LDFLAGS = $(ARCH_CPPFLAGS) -g -O0 $(LIB_DIRS) $(LDSTRIP) $(ARCH_LDFLAGS)

# Source files - modify this part if you want to add new source files (*.c *.cpp *.cc).
# Use make_utils to collect source files
# To recursively add all files in a directory, use:
# $(call mku_add_src_dir, <a directory>)
# To recursively add all files in a directory, except some files, use:
# $(call mku_add_src_dir_excluding, <a directory>, <exclude file list>)
# To recursively add only some files in a directory, use:
# $(call mku_add_src_dir_including, <a directory>, <include file list>)
# All directories are specified relatively to this Makefile working dir (project root).

# Library sources
LIB_SRC = $(call mku_add_src_dir, src)
LIB_SRC += $(call mku_add_src_dir, $(LIB_NETLIB)/src)
LIB_SRC += $(call mku_add_src_dir, $(LIB_MBEDTLS)/library)
LIB_SRC += $(call mku_add_src_dir_including, $(LIB_HTTP_PARSER), http_parser.c)
CRYPTO_SRC = aes.c big.c ecp.c ecp2.c ff.c fp.c fp2.c fp4.c fp12.c gcm.c hash.c mpin.c oct.c pair.c rand.c randapi.c rom.c sok.c
LIB_SRC += $(call mku_add_src_dir_including, $(LIB_CRYPTO)/src, $(CRYPTO_SRC))
LIB_SRC += $(call mku_add_src_dir, $(LIB_PAHO)/MQTTPacket/src)

# Test sources
TEST_SRC = $(call mku_add_src_dir, tests/iot_client)

# The default target
all: staticlib tests

staticlib: $(OUTPUT_LIB)

# Rule for building all tests
tests: $(OUTPUT_TEST)

# Use make_utils to generate everything needed to build the static lib
$(call mku_add_static_lib, $(OUTPUT_LIB), $(LIB_SRC), $(TMP_DIR))

# Use make_utils to generate everything needed to build the test executable
$(call mku_add_executable, $(OUTPUT_TEST), $(TEST_SRC), $(TMP_DIR), $(OUTPUT_LIB))

# Default crosscompile variables (can be set from the env)
ARM_CC = arm-linux-gnueabi-gcc
ARM_CXX = arm-linux-gnueabi-g++
ARM_AR = arm-linux-gnueabi-ar
ARMHF_CC = arm-linux-gnueabihf-gcc
ARMHF_CXX = arm-linux-gnueabihf-g++
ARMHF_AR = arm-linux-gnueabihf-ar

# Targets for crosscompiling
i386:
	$(MAKE) ARCH=i386 ARCH_CPPFLAGS=-m32

x64:
	$(MAKE) ARCH=x64 ARCH_CPPFLAGS=-m64

arm:
	$(MAKE) ARCH=arm CC=$(ARM_CC) CXX=$(ARM_CXX) AR=$(ARM_AR)

armhf:
	$(MAKE) ARCH=armhf CC=$(ARMHF_CC) CXX=$(ARMHF_CXX) AR=$(ARMHF_AR)

crosscompile: i386 x64 arm armhf

# Clean target
clean:
	rm -f -R output/**
