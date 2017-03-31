/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "drm-vts-vendor-modules"

#include <dirent.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <memory>

#include "shared_library.h"
#include "vendor_modules.h"

using std::string;
using std::vector;
using std::unique_ptr;

namespace drm_vts {
vector<string> VendorModules::getVendorModulePaths() {
    if (mModuleList.size() > 0) {
        return mModuleList;
    }

    DIR* dir = opendir(mModulesPath.c_str());
    if (dir == NULL) {
        ALOGE("Unable to open drm VTS vendor directory %s",
              mModulesPath.c_str());
        return mModuleList;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        string fullpath = mModulesPath + "/" + entry->d_name;
        if (endsWith(fullpath, ".so")) {
            mModuleList.push_back(fullpath);
        }
    }

    closedir(dir);
    return mModuleList;
}

DrmHalVTSVendorModule* VendorModules::getVendorModule(const string& path) {
    unique_ptr<SharedLibrary>& library = mOpenLibraries[path];
    if (!library) {
        library = unique_ptr<SharedLibrary>(new SharedLibrary(path));
        if (!library) {
            ALOGE("failed to map shared library %s", path.c_str());
            return NULL;
        }
    }
    void* symbol = library->lookup("vendorModuleFactory");
    if (symbol == NULL) {
        ALOGE("getVendorModule failed to lookup 'vendorModuleFactory' in %s: "
              "%s",
              path.c_str(), library->lastError());
        return NULL;
    }
    typedef DrmHalVTSVendorModule* (*ModuleFactory)();
    ModuleFactory moduleFactory = reinterpret_cast<ModuleFactory>(symbol);
    return (*moduleFactory)();
}
};
