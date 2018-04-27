#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

##### Input Variables:
# LOCAL_MODULE: required. Module name for the build system.
# LOCAL_MODULE_CLASS: optional. Default is ETC.
# LOCAL_MODULE_PATH: optional. Path of output file. Default is $(TARGET_OUT)/etc/vintf.
# LOCAL_MODULE_STEM: optional. Name of output file. Default is $(LOCAL_MODULE).
# LOCAL_SRC_FILES: required. Local source files provided to assemble_vintf
#       (command line argument -i).
# LOCAL_GENERATED_SOURCES: optional. Global source files provided to assemble_vintf
#       (command line argument -i).
#
# LOCAL_ADD_VBMETA_VERSION: Use AVBTOOL to add avb version to the output matrix
#       (corresponds to <avb><vbmeta-version> tag)
# LOCAL_ASSEMBLE_VINTF_ENV_VARS: Add a list of environment variable names from global variables in
#       the build system that is lazily evaluated (e.g. PRODUCT_ENFORCE_VINTF_MANIFEST).
# LOCAL_ASSEMBLE_VINTF_ENV_VARS_OVERRIDE: Add a list of environment variables that is local to
#       assemble_vintf invocation. Format is "VINTF_ENFORCE_NO_UNUSED_HALS=true".
# LOCAL_ASSEMBLE_VINTF_FLAGS: Add additional command line arguments to assemble_vintf invocation.
# LOCAL_KERNEL_CONFIG_DATA_PATHS: Paths to search for kernel config requirements. Format for each is
#       <kernel version x.y.z>:<path that contains android-base*.cfg>.
# LOCAL_GEN_FILE_DEPENDENCIES: A list of additional dependencies for the generated file.

ifndef LOCAL_MODULE
$(error LOCAL_MODULE must be defined.)
endif

ifndef LOCAL_MODULE_STEM
LOCAL_MODULE_STEM := $(LOCAL_MODULE)
endif

ifndef LOCAL_MODULE_CLASS
LOCAL_MODULE_CLASS := ETC
endif

ifndef LOCAL_MODULE_PATH
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/vintf
endif

GEN := $(local-generated-sources-dir)/$(LOCAL_MODULE_STEM)

$(GEN): PRIVATE_ENV_VARS := $(LOCAL_ASSEMBLE_VINTF_ENV_VARS)
$(GEN): PRIVATE_FLAGS := $(LOCAL_ASSEMBLE_VINTF_FLAGS)

$(GEN): $(LOCAL_GEN_FILE_DEPENDENCIES)

ifeq (true,$(strip $(LOCAL_ADD_VBMETA_VERSION)))
ifeq (true,$(BOARD_AVB_ENABLE))
$(GEN): $(AVBTOOL)
# INTERNAL_AVB_SYSTEM_SIGNING_ARGS consists of BOARD_AVB_SYSTEM_KEY_PATH and
# BOARD_AVB_SYSTEM_ALGORITHM. We should add the dependency of key path, which
# is a file, here.
$(GEN): $(BOARD_AVB_SYSTEM_KEY_PATH)
# Use deferred assignment (=) instead of immediate assignment (:=).
# Otherwise, cannot get INTERNAL_AVB_SYSTEM_SIGNING_ARGS.
$(GEN): FRAMEWORK_VBMETA_VERSION = $$("$(AVBTOOL)" add_hashtree_footer \
                           --print_required_libavb_version \
                           $(INTERNAL_AVB_SYSTEM_SIGNING_ARGS) \
                           $(BOARD_AVB_SYSTEM_ADD_HASHTREE_FOOTER_ARGS))
else
$(GEN): FRAMEWORK_VBMETA_VERSION := 0.0
endif # BOARD_AVB_ENABLE
$(GEN): PRIVATE_ENV_VARS += FRAMEWORK_VBMETA_VERSION
endif # LOCAL_ADD_VBMETA_VERSION

ifneq (,$(strip $(LOCAL_KERNEL_CONFIG_DATA_PATHS)))
$(GEN): PRIVATE_KERNEL_CONFIG_DATA_PATHS := $(LOCAL_KERNEL_CONFIG_DATA_PATHS)
$(GEN): $(foreach pair,$(PRIVATE_KERNEL_CONFIG_DATA_PATHS),\
    $(wildcard $(call word-colon,2,$(pair))/android-base*.cfg))
$(GEN): PRIVATE_FLAGS += $(foreach pair,$(PRIVATE_KERNEL_CONFIG_DATA_PATHS),\
	--kernel=$(call word-colon,1,$(pair)):$(call normalize-path-list,\
		$(wildcard $(call word-colon,2,$(pair))/android-base*.cfg)))
endif

my_matrix_src_files := \
	$(addprefix $(LOCAL_PATH)/,$(LOCAL_SRC_FILES)) \
	$(LOCAL_GENERATED_SOURCES)

$(GEN): PRIVATE_ADDITIONAL_ENV_VARS := $(LOCAL_ASSEMBLE_VINTF_ENV_VARS_OVERRIDE)

ifneq (,$(strip $(LOCAL_ASSEMBLE_VINTF_ERROR_MESSAGE)))
$(GEN): PRIVATE_COMMAND_TAIL := || (echo $(strip $(LOCAL_ASSEMBLE_VINTF_ERROR_MESSAGE)) && false)
endif

$(GEN): PRIVATE_SRC_FILES := $(my_matrix_src_files)
$(GEN): $(my_matrix_src_files) $(HOST_OUT_EXECUTABLES)/assemble_vintf
	$(foreach varname,$(PRIVATE_ENV_VARS),\
		$(if $(findstring $(varname),$(PRIVATE_ADDITIONAL_ENV_VARS)),\
			$(error $(varname) should not be overridden by LOCAL_ASSEMBLE_VINTF_ENV_VARS_OVERRIDE.)))
	$(foreach varname,$(PRIVATE_ENV_VARS),$(varname)="$($(varname))") \
		$(PRIVATE_ADDITIONAL_ENV_VARS) \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf \
		-i $(call normalize-path-list,$(PRIVATE_SRC_FILES)) \
		-o $@ \
		$(PRIVATE_FLAGS) $(PRIVATE_COMMAND_TAIL)

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
LOCAL_SRC_FILES :=
LOCAL_GENERATED_SOURCES :=

include $(LOCAL_PATH)/clear_vars.mk
my_matrix_src_files :=

include $(BUILD_PREBUILT)
