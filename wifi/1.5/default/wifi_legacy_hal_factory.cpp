/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <android-base/logging.h>
#include <dlfcn.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "wifi_legacy_hal_factory.h"
#include "wifi_legacy_hal_stubs.h"

namespace {
static constexpr char kVendorHalsDescPath[] = "/vendor/etc/wifi/vendor_hals";
static constexpr char kVendorHalsDescExt[] = ".xml";
static constexpr uint32_t kVendorHalsDescVersion = 1;

bool isDirectory(struct dirent* entryPtr) {
    bool isDir = false;
    if (entryPtr->d_type != DT_UNKNOWN && entryPtr->d_type != DT_LNK) {
        isDir = (entryPtr->d_type == DT_DIR);
    } else {
        struct stat entryStat;
        stat(entryPtr->d_name, &entryStat);
        isDir = S_ISDIR(entryStat.st_mode);
    }
    return isDir;
}

bool isFileExtension(const char* name, const char* ext) {
    if (name == NULL) return false;
    if (ext == NULL) return false;

    size_t extLen = strlen(ext);
    size_t nameLen = strlen(name);

    if (extLen > nameLen) return false;

    if (strncmp(name + nameLen - extLen, ext, extLen) != 0) return false;

    return true;
}
};  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
namespace legacy_hal {

WifiLegacyHalFactory::WifiLegacyHalFactory(
    const std::weak_ptr<wifi_system::InterfaceTool> iface_tool)
    : iface_tool_(iface_tool) {}

std::vector<std::shared_ptr<WifiLegacyHal>> WifiLegacyHalFactory::getHals() {
    if (legacy_hals_.empty()) {
        if (!initVendorHalDescriptorFromLinked())
            initVendorHalsDescriptorList();
        for (auto& desc : descs_) {
            std::shared_ptr<WifiLegacyHal> hal =
                std::make_shared<WifiLegacyHal>(iface_tool_, desc.fn,
                                                desc.primary);
            legacy_hals_.push_back(hal);
        }
    }

    return legacy_hals_;
}

bool WifiLegacyHalFactory::initVendorHalDescriptorFromLinked() {
    wifi_hal_lib_desc desc;

    if (!initLinkedHalFunctionTable(&desc.fn)) return false;

    desc.primary = true;
    desc.handle = NULL;
    descs_.push_back(desc);
    return true;
}

bool WifiLegacyHalFactory::initLinkedHalFunctionTable(wifi_hal_fn* hal_fn) {
    init_wifi_vendor_hal_func_table_t initfn;

    initfn = (init_wifi_vendor_hal_func_table_t)dlsym(
        RTLD_DEFAULT, "init_wifi_vendor_hal_func_table");
    if (!initfn) {
        LOG(INFO) << "no vendor HAL library linked, will try dynamic load";
        return false;
    }

    if (!initHalFuncTableWithStubs(hal_fn)) {
        LOG(ERROR) << "Can not initialize the basic function pointer table";
        return false;
    }

    if (initfn(hal_fn) != WIFI_SUCCESS) {
        LOG(ERROR) << "Can not initialize the vendor function pointer table";
        return false;
    }

    return true;
}

/*
 * Overall structure of the HAL descriptor XML schema
 *
 * <?xml version="1.0" encoding="UTF-8"?>
 * <WifiVendorHal version="1">
 * <path>/vendor/lib64/libwifi-hal-qcom.so</path>
 * <primary>1</primary>
 * </WifiVendorHal>
 */
void WifiLegacyHalFactory::initVendorHalsDescriptorList() {
    xmlDocPtr xml;
    xmlNodePtr node, cnode;
    char* version;
    std::string path;
    xmlChar* value;
    wifi_hal_lib_desc desc;

    LOG(INFO) << "processing vendor HALs descriptions in "
              << kVendorHalsDescPath;
    DIR* dirPtr = ::opendir(kVendorHalsDescPath);
    if (dirPtr == NULL) {
        LOG(ERROR) << "failed to open " << kVendorHalsDescPath;
        return;
    }
    for (struct dirent* entryPtr = ::readdir(dirPtr); entryPtr != NULL;
         entryPtr = ::readdir(dirPtr)) {
        if (isDirectory(entryPtr)) continue;

        if (!isFileExtension(entryPtr->d_name, kVendorHalsDescExt))
            continue;  // only process .xml files

        LOG(INFO) << "processing config file: " << entryPtr->d_name;

        std::string fullPath(kVendorHalsDescPath);
        fullPath.append("/");
        fullPath.append(entryPtr->d_name);
        xml = xmlReadFile(fullPath.c_str(), "UTF-8", XML_PARSE_RECOVER);
        if (!xml) {
            LOG(ERROR) << "failed to parse: " << entryPtr->d_name
                       << " skipping...";
            continue;
        }
        node = xmlDocGetRootElement(xml);
        if (!node) {
            LOG(ERROR) << "empty config file: " << entryPtr->d_name
                       << " skipping...";
            goto skip;
        }
        if (xmlStrcmp(node->name, BAD_CAST "WifiVendorHal")) {
            LOG(ERROR) << "bad config, root element not WifiVendorHal: "
                       << entryPtr->d_name << " skipping...";
            goto skip;
        }
        version = (char*)xmlGetProp(node, BAD_CAST "version");
        if (!version || strtoul(version, NULL, 0) != kVendorHalsDescVersion) {
            LOG(ERROR) << "conf file: " << entryPtr->d_name
                       << "must have version: " << kVendorHalsDescVersion
                       << ", skipping...";
            goto skip;
        }
        cnode = node->children;
        path.clear();
        desc.primary = false;
        while (cnode) {
            if (!xmlStrcmp(cnode->name, BAD_CAST "path")) {
                value = xmlNodeListGetString(xml, cnode->children, 1);
                if (value) path = (char*)value;
                xmlFree(value);
            } else if (!xmlStrcmp(cnode->name, BAD_CAST "primary")) {
                value = xmlNodeListGetString(xml, cnode->children, 1);
                desc.primary = !xmlStrcmp(value, BAD_CAST "1");
                xmlFree(value);
            }
            cnode = cnode->next;
        }
        if (path.empty()) {
            LOG(ERROR) << "hal library path not provided in: "
                       << entryPtr->d_name << ", skipping...";
            goto skip;
        }
        if (loadVendorHalLib(path, desc)) {
            if (desc.primary)
                descs_.insert(descs_.begin(), desc);
            else
                descs_.push_back(desc);
        }
    skip:
        xmlFreeDoc(xml);
    }
    ::closedir(dirPtr);
}

bool WifiLegacyHalFactory::loadVendorHalLib(const std::string& path,
                                            wifi_hal_lib_desc& desc) {
    void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    init_wifi_vendor_hal_func_table_t initfn;
    wifi_error res;

    if (!h) {
        LOG(ERROR) << "failed to open vendor hal library: " << path;
        return false;
    }
    initfn = (init_wifi_vendor_hal_func_table_t)dlsym(
        h, "init_wifi_vendor_hal_func_table");
    if (!initfn) {
        LOG(ERROR) << "init_wifi_vendor_hal_func_table not found in: " << path;
        goto out_err;
    }

    if (!initHalFuncTableWithStubs(&desc.fn)) {
        LOG(ERROR) << "Can not initialize the basic function pointer table";
        goto out_err;
    }
    res = initfn(&desc.fn);
    if (res != WIFI_SUCCESS) {
        LOG(ERROR) << "failed to initialize the vendor func table in: " << path
                   << " error: " << res;
        goto out_err;
    }

    res = desc.fn.wifi_early_initialize();
    // vendor HALs which do not implement early_initialize will return
    // WIFI_ERROR_NOT_SUPPORTED, treat this as success.
    if (res != WIFI_SUCCESS && res != WIFI_ERROR_NOT_SUPPORTED) {
        LOG(ERROR) << "early initialization failed in: " << path
                   << " error: " << res;
        goto out_err;
    }

    desc.handle = h;
    return true;
out_err:
    dlclose(h);
    return false;
}

}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android
