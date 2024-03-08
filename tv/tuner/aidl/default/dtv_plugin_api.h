#ifndef LIVE_DTV_PLUGIN_API_H_
#define LIVE_DTV_PLUGIN_API_H_

#include <stdint.h>

struct dtv_streamer;

struct dtv_plugin {
    uint32_t version;

    /**
     * get_transport_types() - Retrieve a list of supported transport types.
     *
     * Return: A NULL-terminated list of supported transport types.
     */
    const char** (*get_transport_types)(void);

    /**
     * get_streamer_count() - Get number of streamers that can be created.
     *
     * Return: The number of streamers that can be created.
     */
    int (*get_streamer_count)(void);

    /**
     * validate() - Check if transport description is valid.
     * @transport_desc: NULL-terminated transport description in json format.
     *
     * Return: 1 if valid, 0 otherwise.
     */
    int (*validate)(const char* transport_desc);

    /**
     * create_streamer() - Create a streamer object.
     *
     * Return: A pointer to a new streamer object.
     */
    struct dtv_streamer* (*create_streamer)(void);

    /**
     * destroy_streamer() - Free a streamer object and all associated resources.
     * @st: Pointer to a streamer object
     */
    void (*destroy_streamer)(struct dtv_streamer* streamer);

    /**
     * set_property() - Set a key/value pair property.
     * @streamer: Pointer to a streamer object (may be NULL for plugin-wide properties).
     * @key: NULL-terminated property name.
     * @value: Property value.
     * @size: Property value size.
     *
     * Return: 0 if success, -1 otherwise.
     */
    int (*set_property)(struct dtv_streamer* streamer, const char* key, const void* value,
                        size_t size);

    /**
     * get_property() - Get a property's value.
     * @streamer: Pointer to a streamer (may be NULL for plugin-wide properties).
     * @key: NULL-terminated property name.
     * @value: Property value.
     * @size: Property value size.
     *
     * Return: >= 0 if success, -1 otherwise.
     *
     * If size is 0, get_property will return the size needed to hold the value.
     */
    int (*get_property)(struct dtv_streamer* streamer, const char* key, void* value, size_t size);

    /**
     * add_pid() - Add a TS filter on a given pid.
     * @streamer: The streamer that outputs the TS.
     * @pid: The pid to add to the TS output.
     *
     * Return: 0 if success, -1 otherwise.
     *
     * This function is optional but can be useful if a hardware remux is
     * available.
     */
    int (*add_pid)(struct dtv_streamer* streamer, int pid);

    /**
     * remove_pid() - Remove a TS filter on a given pid.
     * @streamer: The streamer that outputs the TS.
     * @pid: The pid to remove from the TS output.
     *
     * Return: 0 if success, -1 otherwise.
     *
     * This function is optional.
     */
    int (*remove_pid)(struct dtv_streamer* streamer, int pid);

    /**
     * open_stream() - Open a stream from a transport description.
     * @streamer: The streamer which will handle the stream.
     * @transport_desc: NULL-terminated transport description in json format.
     *
     * The streamer will allocate the resources and make the appropriate
     * connections to handle this transport.
     * This function returns a file descriptor that can be polled for events.
     *
     * Return: A file descriptor if success, -1 otherwise.
     */
    int (*open_stream)(struct dtv_streamer* streamer, const char* transport_desc);

    /**
     * close_stream() - Release an open stream.
     * @streamer: The streamer from which the stream should be released.
     */
    void (*close_stream)(struct dtv_streamer* streamer);

    /**
     * read_stream() - Read stream data.
     * @streamer: The streamer to read from.
     * @buf: The destination buffer.
     * @count: The number of bytes to read.
     * @timeout_ms: Timeout in ms.
     *
     * Return: The number of bytes read, -1 if error.
     */
    ssize_t (*read_stream)(struct dtv_streamer* streamer, void* buf, size_t count, int timeout_ms);
};

struct dtv_plugin_event {
    int id;
    char data[0];
};

enum {
    DTV_PLUGIN_EVENT_SIGNAL_LOST = 1,
    DTV_PLUGIN_EVENT_SIGNAL_READY,
};

#define PROPERTY_STATISTICS "statistics"

#endif  // LIVE_DTV_PLUGIN_API_H_
