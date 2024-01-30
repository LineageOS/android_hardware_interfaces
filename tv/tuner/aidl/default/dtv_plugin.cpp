#include "dtv_plugin.h"
#include <dlfcn.h>
#include <libgen.h>
#include <utils/Log.h>

DtvPlugin::DtvPlugin(const char* plugin_path) {
    path_ = plugin_path;
    basename_ = basename(path_);
    module_ = NULL;
    interface_ = NULL;
    loaded_ = false;
}

DtvPlugin::~DtvPlugin() {
    if (module_ != NULL) {
        if (dlclose(module_)) ALOGE("DtvPlugin: Failed to close plugin '%s'", basename_);
    }
}

bool DtvPlugin::load() {
    ALOGI("Loading plugin '%s' from path '%s'", basename_, path_);

    module_ = dlopen(path_, RTLD_LAZY);
    if (module_ == NULL) {
        ALOGE("DtvPlugin::Load::Failed to load plugin '%s'", basename_);
        ALOGE("dlopen error: %s", dlerror());
        return false;
    }

    interface_ = (dtv_plugin*)dlsym(module_, "plugin_entry");

    if (interface_ == NULL) {
        ALOGE("plugin_entry is NULL.");
        goto error;
    }

    if (!interface_->get_transport_types || !interface_->get_streamer_count ||
        !interface_->validate || !interface_->create_streamer || !interface_->destroy_streamer ||
        !interface_->open_stream || !interface_->close_stream || !interface_->read_stream) {
        ALOGW("Plugin: missing one or more callbacks");
        goto error;
    }

    loaded_ = true;

    return true;

error:
    if (dlclose(module_)) ALOGE("Failed to close plugin '%s'", basename_);

    return false;
}

int DtvPlugin::getStreamerCount() {
    if (!loaded_) {
        ALOGE("DtvPlugin::GetStreamerCount: Plugin '%s' not loaded!", basename_);
        return 0;
    }

    return interface_->get_streamer_count();
}

bool DtvPlugin::isTransportTypeSupported(const char* transport_type) {
    const char** transport;

    if (!loaded_) {
        ALOGE("Plugin '%s' not loaded!", basename_);
        return false;
    }

    transport = interface_->get_transport_types();
    if (transport == NULL) return false;

    while (*transport) {
        if (strcmp(transport_type, *transport) == 0) return true;
        transport++;
    }

    return false;
}

bool DtvPlugin::validate(const char* transport_desc) {
    if (!loaded_) {
        ALOGE("Plugin '%s' is not loaded!", basename_);
        return false;
    }

    return interface_->validate(transport_desc);
}

bool DtvPlugin::getProperty(const char* key, void* value, int* size) {
    if (!loaded_) {
        ALOGE("Plugin '%s' is not loaded!", basename_);
        return false;
    }

    if (!interface_->get_property) return false;

    *size = interface_->get_property(NULL, key, value, *size);

    return *size < 0 ? false : true;
}

bool DtvPlugin::setProperty(const char* key, const void* value, int size) {
    int ret;

    if (!loaded_) {
        ALOGE("Plugin '%s': not loaded!", basename_);
        return false;
    }

    if (!interface_->set_property) return false;

    ret = interface_->set_property(NULL, key, value, size);

    return ret < 0 ? false : true;
}

struct dtv_plugin* DtvPlugin::interface() {
    if (!loaded_) {
        ALOGE("Plugin '%s' is not loaded!", basename_);
        return NULL;
    }

    return interface_;
}

const char* DtvPlugin::pluginBasename() {
    return basename_;
}
