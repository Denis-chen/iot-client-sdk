#
# Make Utils Functions
#

#
# mku_src_find
# args:
# $(1) - source directory (relative to Makefile current dir)
# returns list of all C/C++ files in the specified directory
define mku_src_find
$(shell find $(1) \( -name "*.cpp" -o -name "*.cc" -o -name "*.c" \))
endef
#
# mku_add_src_dir
# args:
# $(1) - source directory (relative to Makefile current dir)
define mku_add_src_dir
$(call mku_src_find,$(strip $(1)))
endef
#
# mku_filter_src
# args:
# $(1) - filter function to use
# $(2) - source directory (relative to Makefile current dir)
# $(3) - list of files (prefixed with % and used as patterns) for the filter function
define mku_filter_src
$(call $(1),$(addprefix %,$(3)),$(call mku_add_src_dir,$(2)))
endef
#
# mku_add_src_dir_excluding
# args:
# $(1) - source directory (relative to Makefile current dir)
# $(2) - list of files (prefixed with % and used as patterns) to exclude
define mku_add_src_dir_excluding
$(call mku_filter_src,filter-out,$(1),$(2))
endef
#
# mku_add_src_dir_including
# args:
# $(1) - source directory (relative to Makefile current dir)
# $(2) - list of files (prefixed with % and used as patterns) to include
define mku_add_src_dir_including
$(call mku_filter_src,filter,$(1),$(2))
endef
#
# mku_src_to_obj
# args:
# $(1) - single source file pathname (relative to Makefile current dir)
# $(2) - object files (temporaries) root location (relative to Makefile current dir)
define mku_src_to_obj
$(strip $(2))/$(subst ../,,$(basename $(1))).o
endef
#
# mku_src_to_obj_list
# args:
# $(1) - list of source file pathnames (relative to Makefile current dir)
# $(2) - object files (temporaries) root location (relative to Makefile current dir)
define mku_src_to_obj_list
$(foreach src_file,$(1),$(call mku_src_to_obj,$(src_file),$(2)))
endef
#
# mku_compile
# args:
# $(1) - single source file pathname (relative to Makefile current dir)
# $(2) - object files (temporaries) root location (relative to Makefile current dir)
define mku_compile
$(call mku_src_to_obj,$(1),$(2)): $(1)
	$$(shell mkdir -p $$(dir $$@))
ifeq ($(suffix $(1)),.c)
	$$(CC) -MMD -MP $$(CFLAGS) $$(CPPFLAGS) -c $$< -o $$@
else
	$$(CXX) -MMD -MP $$(CXXFLAGS) $$(CPPFLAGS) -c $$< -o $$@
endif
endef
#
# mku_get_ldflags
# args:
# $(1) - list of library pathnames (relative to Makefile current dir)
define mku_get_ldflags
$(addprefix -L,$(dir $(1)))
endef
#
# mku_get_ldlibs
# args:
# $(1) - list of library pathnames (relative to Makefile current dir)
define mku_get_ldlibs
$(foreach lib,$(1),$(addprefix -l,$(patsubst lib%$(suffix $(lib)),%,$(notdir $(lib)))))
endef
#
# mku_add_executable_template
# args:
# $(1) - output pathname (relative to Makefile current dir)
# $(2) - list of source file pathnames (relative to Makefile current dir)
# $(3) - object files (temporaries) root location (relative to Makefile current dir)
# $(4) - list of dependencies (library pathnames, relative to Makefile current dir)
define mku_add_executable_template
$(1): $(call mku_src_to_obj_list,$(2),$(3)) $(4)
	$$(shell mkdir -p $$(dir $$@))
	$$(CXX) -o $$@ $$^ $(call mku_get_ldflags,$(4)) $$(LDFLAGS) $(call mku_get_ldlibs,$(4)) $$(LDLIBS)
$(foreach src_file,$(2),$(eval $(call mku_compile,$(src_file),$(3))))
-include $($(call mku_src_to_obj_list,$(2),$(3)):%.o=%.d)
endef
#
# mku_add_executable
# args:
# $(1) - output pathname (relative to Makefile current dir)
# $(2) - list of source file pathnames (relative to Makefile current dir)
# $(3) - object files (temporaries) root location (relative to Makefile current dir)
# $(4) - dependencies (library pathnames, relative to Makefile current dir)
define mku_add_executable
$(eval $(call mku_add_executable_template,$(strip $(1)),$(strip $(2)),$(strip $(3)),$(strip $(4))))
endef
#
# mku_add_static_lib_template
# args:
# $(1) - output pathname (relative to Makefile current dir)
# $(2) - list of source file pathnames (relative to Makefile current dir)
# $(3) - object files (temporaries) root location (relative to Makefile current dir)
define mku_add_static_lib_template
$(1): $(call mku_src_to_obj_list,$(2),$(3))
	$$(shell mkdir -p $$(dir $$@))
	$$(AR) rcs $$@ $$^
$(foreach src_file,$(2),$(eval $(call mku_compile,$(src_file),$(3))))
-include $($(call mku_src_to_obj_list,$(2),$(3)):%.o=%.d)
endef
#
# mku_add_static_lib
# args:
# $(1) - output pathname (relative to Makefile current dir)
# $(2) - list of source file pathnames (relative to Makefile current dir)
# $(3) - object files (temporaries) root location (relative to Makefile current dir)
define mku_add_static_lib
$(eval $(call mku_add_static_lib_template,$(strip $(1)),$(strip $(2)),$(strip $(3))))
endef
