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

###########################################################
## Remove minor revision from a kernel version. For example,
## 3.18.0 becomes 3.18.
## $(1): kernel version
###########################################################
define remove-minor-revision
$(strip $(subst $(space),.,$(wordlist 1,2,$(subst .,$(space),$(strip $(1))))))
endef

# $(warning $(call remove-minor-revision,3.18.0))

ifndef LOCAL_MODULE_STEM
$(error LOCAL_MODULE_STEM must be defined.)
endif

LOCAL_MODULE := framework_$(LOCAL_MODULE_STEM)
LOCAL_MODULE_CLASS := ETC

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

ifneq (,$(strip $(LOCAL_KERNEL_VERSIONS)))
$(GEN): PRIVATE_KERNEL_CONFIG_DATA := kernel/configs
$(GEN): PRIVATE_KERNEL_VERSIONS := $(LOCAL_KERNEL_VERSIONS)
$(GEN): $(foreach version,$(PRIVATE_KERNEL_VERSIONS),\
    $(wildcard $(PRIVATE_KERNEL_CONFIG_DATA)/android-$(call remove-minor-revision,$(version))/android-base*.cfg))
$(GEN): PRIVATE_FLAGS += $(foreach version,$(PRIVATE_KERNEL_VERSIONS),\
    --kernel=$(version):$(call normalize-path-list,\
        $(wildcard $(PRIVATE_KERNEL_CONFIG_DATA)/android-$(call remove-minor-revision,$(version))/android-base*.cfg)))
endif

my_matrix_src_files := \
	$(addprefix $(LOCAL_PATH)/,$(LOCAL_SRC_FILES)) \
	$(LOCAL_GENERATED_SOURCES)

$(GEN): PRIVATE_SRC_FILES := $(my_matrix_src_files)
$(GEN): $(my_matrix_src_files) $(HOST_OUT_EXECUTABLES)/assemble_vintf
	$(foreach varname,$(PRIVATE_ENV_VARS),$(varname)="$($(varname))") \
		$(HOST_OUT_EXECUTABLES)/assemble_vintf \
		-i $(call normalize-path-list,$(PRIVATE_SRC_FILES)) \
		-o $@ \
		$(PRIVATE_FLAGS)

LOCAL_PREBUILT_MODULE_FILE := $(GEN)
LOCAL_SRC_FILES :=
LOCAL_GENERATED_SOURCES :=

LOCAL_ADD_VBMETA_VERSION :=
LOCAL_ASSEMBLE_VINTF_ENV_VARS :=
LOCAL_ASSEMBLE_VINTF_FLAGS :=
LOCAL_KERNEL_VERSIONS :=
LOCAL_GEN_FILE_DEPENDENCIES :=
my_matrix_src_files :=

include $(BUILD_PREBUILT)

remove-minor-revision :=
