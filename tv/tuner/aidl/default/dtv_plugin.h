#ifndef LIVE_DTV_PLUGIN_H_
#define LIVE_DTV_PLUGIN_H_

#include <fstream>
#include "dtv_plugin_api.h"

class DtvPlugin {
  public:
    DtvPlugin(const char* plugin_path);
    ~DtvPlugin();

    bool load();
    int getStreamerCount();
    bool validate(const char* transport_desc);
    bool isTransportTypeSupported(const char* transport_type);
    //    /* plugin-wide properties */
    bool getProperty(const char* key, void* value, int* size);
    bool setProperty(const char* key, const void* value, int size);

    struct dtv_plugin* interface();
    const char* pluginBasename();

  protected:
    const char* path_;
    char* basename_;
    void* module_;
    struct dtv_plugin* interface_;
    bool loaded_;
};

#endif  // LIVE_DTV_PLUGIN_H_
