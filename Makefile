# Directories: ROOT_DIR - root of all sources, TMP_DIR - temporary build files, OUTPUT_DIR - final binaries dir
ROOT_DIR = .
TMP_DIR = $(ROOT_DIR)/output$(ARCH)/obj
OUTPUT_DIR = $(ROOT_DIR)/output$(ARCH)

# Output file
OUTPUT_LIB_NAME = iotclient
OUTPUT_FILE = $(OUTPUT_DIR)/lib$(OUTPUT_LIB_NAME).a

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
	-I$(ROOT_DIR)/include \
	-I$(ROOT_DIR)/$(LIB_CAJUN) \
	-I$(ROOT_DIR)/$(LIB_FMT) \
	-I$(ROOT_DIR)/$(LIB_NETLIB)/include \
	-I$(ROOT_DIR)/$(LIB_MBEDTLS)/include \
	-I$(ROOT_DIR)/$(LIB_HTTP_PARSER) \
	-I$(ROOT_DIR)/$(LIB_CRYPTO)/include \
	-I$(ROOT_DIR)/$(LIB_PAHO)/MQTTClient/src -I$(ROOT_DIR)/$(LIB_PAHO)/MQTTPacket/src

# Uncomment following two lines to enable unused symbols stripping
#CPPSTRIP = -fdata-sections -ffunction-sections
#LDSTRIP = -Wl,--gc-sections

# C and C++ flags
CPPFLAGS += $(ARCH_FLAGS) -g -MMD -MP $(INCLUDE_DIRS) -DFMT_HEADER_ONLY -DC99 $(CPPSTRIP)
CXXFLAGS += -std=c++98
#CFLAGS += -std=c99

# Additional library search paths
LIB_DIRS =

# Additional libraries
LDLIBS =

# Linker flags
LDFLAGS = $(LIB_DIRS) $(LDSTRIP)

# Utility functions, used later in build
# rfind
define rfind
$(shell find $(1) -name '$(2)')
endef
# add_src_dir
define add_src_dir
$(sort $(call rfind,$(ROOT_DIR)/$(strip $(1)),*.c) $(call rfind,$(ROOT_DIR)/$(strip $(1)),*.cpp) $(call rfind,$(ROOT_DIR)/$(strip $(1)),*.cc))
endef
# filter_src
define filter_src
$(call $(1),$(addprefix %,$(3)),$(call add_src_dir,$(2)))
endef
# add_src_dir_excluding
define add_src_dir_excluding
$(call filter_src,filter-out,$(1),$(2))
endef
# add_src_dir_including
define add_src_dir_including
$(call filter_src,filter,$(1),$(2))
endef
# c_to_obj
define c_to_obj
$(patsubst $(ROOT_DIR)/%.c, $(TMP_DIR)/%.o, $(1))
endef
# cpp_to_obj
define cpp_to_obj
$(patsubst $(ROOT_DIR)/%.cpp, $(TMP_DIR)/%.o, $(1))
endef
# cc_to_obj
define cc_to_obj
$(patsubst $(ROOT_DIR)/%.cc, $(TMP_DIR)/%.o, $(1))
endef
# generate_c_rule
define generate_c_rule
$(2): $(1)
	$$(shell mkdir -p $(dir $(2)))
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
endef
# generate_cpp_rule
define generate_cpp_rule
$(2): $(1)
	$$(shell mkdir -p $(dir $(2)))
	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@
endef
# generate_cc_rule
define generate_cc_rule
$(2): $(1)
	$$(shell mkdir -p $(dir $(2)))
	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@
endef

# Source files - modify this part if you want to add new source files (*.c *.cpp).
# To recursively add all files in a directory, use:
# $(call add_src_dir, <a directory>)
# To recursively add all files in a directory, except some files, use:
# $(call add_src_dir_excluding, <a directory>, <exclude file list>)
# To recursively add only some files in a directory, use:
# $(call add_src_dir_including, <a directory>, <include file list>)
# All directories are specified relatively to $(ROOT_DIR).
SRC = $(call add_src_dir, src)
SRC += $(call add_src_dir, $(LIB_NETLIB)/src)
SRC += $(call add_src_dir, $(LIB_MBEDTLS)/library)
SRC += $(call add_src_dir_including, $(LIB_HTTP_PARSER), http_parser.c)
CRYPTO_SRC = aes.c big.c ecp.c ecp2.c ff.c fp.c fp2.c fp4.c fp12.c gcm.c hash.c mpin.c oct.c pair.c rand.c randapi.c rom.c sok.c
SRC += $(call add_src_dir_including, $(LIB_CRYPTO)/src, $(CRYPTO_SRC))
SRC += $(call add_src_dir, $(LIB_PAHO)/MQTTPacket/src)

# Generate a list of object files
OBJ = $(call cc_to_obj, $(call cpp_to_obj, $(call c_to_obj, $(SRC))))

# Separate .c and .cpp files
C_SRC = $(filter %.c, $(SRC))
CPP_SRC = $(filter %.cpp, $(SRC))
CC_SRC = $(filter %.cc, $(SRC))

# The default target
all: staticlib tests

staticlib: $(OUTPUT_FILE)

# Rule for building the output file - depends on all object files
$(OUTPUT_FILE): $(OBJ)
	$(shell mkdir -p $(OUTPUT_DIR))
	$(AR) rcs $@ $^

# Rule for building all tests
tests: iot_client

# Rules for building iot_client test
iot_client: $(OUTPUT_DIR)/iot_client

$(OUTPUT_DIR)/iot_client: $(OUTPUT_FILE) $(ROOT_DIR)/tests/iot_client/main.cpp $(ROOT_DIR)/tests/iot_client/flags.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(ROOT_DIR)/tests/iot_client/*.cpp -L$(OUTPUT_DIR) $(LDFLAGS) -l$(OUTPUT_LIB_NAME) $(LDLIBS)

i386:
	$(MAKE) ARCH=/i386 ARCH_FLAGS=-m32

x64:
	$(MAKE) ARCH=/x64 ARCH_FLAGS=-m64

arm:
	$(MAKE) ARCH=/arm CC=arm-linux-gnueabi-gcc CXX=arm-linux-gnueabi-g++ AR=arm-linux-gnueabi-ar

armhf:
	$(MAKE) ARCH=/armhf CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ AR=arm-linux-gnueabihf-ar

# Generate rules for each object file that depends on the corresponding .c file
$(foreach cfile, $(C_SRC), $(eval $(call generate_c_rule, $(cfile), $(call c_to_obj, $(cfile)))))

# Generate rules for each object file that depends on the corresponding .cpp file
$(foreach cppfile, $(CPP_SRC), $(eval $(call generate_cpp_rule, $(cppfile), $(call cpp_to_obj, $(cppfile)))))

# Generate rules for each object file that depends on the corresponding .cc file
$(foreach ccfile, $(CC_SRC), $(eval $(call generate_cc_rule, $(ccfile), $(call cc_to_obj, $(ccfile)))))

# Clean target
#	rm -f -R $(TMP_DIR)/** $(OUTPUT_FILE) $(TEST_FILES)
clean:
	rm -f -R $(OUTPUT_DIR)/**

# Include all the .d files (generated by the -MMD -MP option) corresponding to each of the object files
# This adds to each object target a dependency on all the header files, included in the corresponding c/cpp/cc file
-include $(OBJ:%.o=%.d)
