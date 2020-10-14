#include <iostream>
#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/stream.h>
#include <pulse/error.h>
#include "PulseAudioThreaded.h"

static void context_state_callback(pa_context *c, void *userdata)
{
    pa_threaded_mainloop *loop = (pa_threaded_mainloop *)userdata;
    fprintf(stdout, "context_state_callback state=%d\n", pa_context_get_state(c));
    pa_threaded_mainloop_signal(loop, 0);
}

void PulseAudioThreaded::stream_state_callback(pa_stream *s, void *userdata)
{
    PulseAudioThreaded *paThreaded = (PulseAudioThreaded *)userdata;
    pa_stream_state_t state = pa_stream_get_state(s);

    fprintf(stdout, "stream_state_callback state=%d\n", state);
    if (state == PA_STREAM_FAILED)
    {
        fprintf(stderr, "Stream failed:%s\n", pa_strerror(pa_context_errno(paThreaded->ctx)));
    }
    pa_threaded_mainloop_signal(paThreaded->loop, 0);
}

static void stream_overflow_callback(pa_stream *s, void *userdata)
{
    fprintf(stderr, "stream_overflow_callback\n");
    const pa_buffer_attr *buf = pa_stream_get_buffer_attr(s);
}

static void stream_write_callback(pa_stream *s, size_t nbytes, void *userdata)
{
    pa_threaded_mainloop *loop = (pa_threaded_mainloop *)userdata;
    fprintf(stdout, "stream_write_callback(, %d, )\n", nbytes);

    pa_threaded_mainloop_signal(loop, 0);
}

static void drain_callback(pa_stream *s, int success, void *userdata)
{
    pa_threaded_mainloop *loop = (pa_threaded_mainloop *)userdata;
    pa_threaded_mainloop_signal(loop, 0);
}

void PulseAudioThreaded::__construct(Php::Parameters &params)
{
    std::cout << "PulseAudioThreaded::__construct:" << params.size() << std::endl;

    pa_stream_direction_t dir = PA_STREAM_NODIRECTION;
    pa_sample_spec spec = {};
    pa_channel_map *map = NULL;
    pa_buffer_attr *buffAttr = NULL;
    int error = 0;
    const char *server = NULL;
    if (!params[0].isNull())
        server = params[0].rawValue();

    const char *name = params[1].rawValue();
    const char *dev = params[3].isNull() ? NULL : params[3].rawValue();
    const char *streamName = params.size() > 4 && !params[4].isNull() ? params[4].rawValue() : NULL;

    Php::Value phpDir = params[2];
    dir = (pa_stream_direction_t)phpDir.numericValue();

    if (params.size() > 5 && !params[5].isNull())
    {
        Php::Value phpSpec = params[5];
        if (phpSpec.size() != 3)
        {
            throw Php::Exception("sample spec must contain 3 elements: channels,rate,format");
        }
        spec.channels = phpSpec.get("channels").numericValue();
        spec.rate = phpSpec.get("rate").numericValue();
        spec.format = (pa_sample_format_t)phpSpec.get("format").numericValue();
    }
    if (params.size() > 6 && !params[6].isNull())
    {
        Php::Value phpMap = params[6];
        map = {};
        map->channels = (int16_t)count(phpMap);
        memcpy(map->map, phpMap, map->channels);
    }

    if (params.size() > 7 && !params[7].isNull())
    {
        Php::Value phpBuffAttr = params[7];
        buffAttr = {};
        buffAttr->fragsize = (int32_t)phpBuffAttr["fragsize"];
        buffAttr->maxlength = (int32_t)phpBuffAttr["maxlength"];
        buffAttr->minreq = (int32_t)phpBuffAttr["minreq"];
        buffAttr->prebuf = (int32_t)phpBuffAttr["prebuf"];
        buffAttr->tlength = (int32_t)phpBuffAttr["tlength"];
    }

    std::cout << "name:" << name << std::endl;
    std::cout << "dir:" << dir << std::endl;
    std::cout << "server: " << (server == NULL ? "null" : server) << std::endl;
    std::cout << "stream name: " << (streamName == NULL ? "null" : streamName) << std::endl;
    std::cout << "spec: " << spec.channels << " " << spec.rate << " " << spec.format << std::endl;
    std::cout << "map: " << (map == NULL ? "null" : std::to_string(map->channels)) << std::endl;
    std::cout << "buffattr: " << (buffAttr == NULL ? "null " : std::to_string(buffAttr->tlength)) << std::endl;

    int ret;
    loop = pa_threaded_mainloop_new();
    if (!loop)
    {
        throw Php::Exception("Cannot allocated mainloop");
    }

    if ((ret = pa_threaded_mainloop_start(loop)) != 0)
    {
        throw Php::Exception("Cannot start threaded_mainloop: " + std::string(pa_strerror(ret)));
    }

    pa_threaded_mainloop_lock(loop);

    pa_mainloop_api *loopApi = pa_threaded_mainloop_get_api(loop);

    ctx = pa_context_new(loopApi, name);
    if (!ctx)
    {
        pa_threaded_mainloop_unlock(loop);
        throw new Php::Exception("Cannot allocate context");
    }

    pa_context_flags_t flags = PA_CONTEXT_NOFLAGS;
    pa_context_set_state_callback(ctx, context_state_callback, loop);
    if ((ret = pa_context_connect(ctx, server, flags, NULL)) != 0)
    {
        pa_threaded_mainloop_unlock(loop);
        throw Php::Exception("Cannot connect context: " + std::string(pa_strerror(ret)));
    }

    while (pa_context_get_state(ctx) != PA_CONTEXT_READY && pa_context_get_state(ctx) != PA_CONTEXT_FAILED)
    {
        pa_threaded_mainloop_wait(loop);
    }

    //Setup stream
    // pa_format_info *info[1];
    // info[0] = pa_format_info_new();
    // info[0]->encoding = PA_ENCODING_ANY;

    // pa_proplist *props = pa_proplist_new();
    // pa_proplist_sets(props, PA_PROP_FORMAT_CHANNELS, "2");
    // pa_proplist_sets(props, PA_PROP_FORMAT_RATE, "41000");
    // pa_proplist_sets(props, PA_PROP_FORMAT_SAMPLE_FORMAT, pa_sample_format_to_string(PA_SAMPLE_S16LE));

    // stream = pa_stream_new_extended(ctx, streamName, info, 1, NULL);
    stream = pa_stream_new(ctx, name, &spec, map);
    if (!stream)
    {
        pa_threaded_mainloop_unlock(loop);
        throw Php::Exception("Cannot allocate stream:" + std::string(pa_strerror(pa_context_errno(ctx))));
    }
    pa_stream_set_state_callback(stream, stream_state_callback, this);
    pa_stream_set_overflow_callback(stream, stream_overflow_callback, NULL);
    pa_stream_set_write_callback(stream, stream_write_callback, loop);

    if ((ret = pa_stream_connect_playback(stream, dev, buffAttr, PA_STREAM_NOFLAGS, NULL, NULL)) != 0)
    {
        pa_threaded_mainloop_unlock(loop);
        throw Php::Exception("Cannot connect playback stream: " + std::string(pa_strerror(ret)));
    }

    while (pa_stream_get_state(stream) != PA_STREAM_READY && pa_stream_get_state(stream) != PA_STREAM_FAILED)
    {
        pa_threaded_mainloop_wait(loop);
    }
    if (pa_stream_get_state(stream) != PA_STREAM_READY)
    {
        pa_threaded_mainloop_unlock(loop);
        throw Php::Exception("Stream failed to get ready: " + std::string(pa_strerror(pa_context_errno(ctx))));
    }

    pa_threaded_mainloop_unlock(loop);
    std::cout << "PulseAudioThreaded::__construct done" << error << std::endl;
}

void PulseAudioThreaded::write(Php::Parameters &params)
{
    const char *data = params[0];
    size_t len = params[0].size();

    pa_threaded_mainloop_lock(loop);

    while (pa_stream_writable_size(stream) == 0)
        pa_threaded_mainloop_wait(loop);

    void *buffer = NULL;
    int error = 0;
    size_t nbytes = (size_t)-1;

    error = pa_stream_begin_write(stream, &buffer, &nbytes);
    if (error != 0 || buffer == NULL)
    {
        pa_threaded_mainloop_unlock(loop);
        throw Php::Exception("Cannot begin writting to pulseaudio: " + std::string(pa_strerror(pa_context_errno(ctx))));
    }
    nbytes = len < nbytes ? len : nbytes;
    // Defeats the point of using _begin_write
    memcpy(buffer, data, nbytes);

    if (pa_stream_write(stream, buffer, nbytes, NULL, 0, PA_SEEK_RELATIVE) != 0)
    {
        pa_threaded_mainloop_unlock(loop);
        throw Php::Exception("Cannot write to pulseaudio: " + std::string(pa_strerror(pa_context_errno(ctx))));
    }
    pa_threaded_mainloop_unlock(loop);
}

void PulseAudioThreaded::drain()
{
    std::cout << "PulseAudioThreaded::draining " << std::endl;
    pa_threaded_mainloop_lock(loop);
    pa_operation *op = pa_stream_drain(stream, drain_callback, loop);
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
    {
        pa_threaded_mainloop_wait(loop);
    }
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(loop);
}

void PulseAudioThreaded::__destruct()
{
    pa_stream_disconnect(stream);
    pa_context_disconnect(ctx);
    pa_threaded_mainloop_stop(loop);
}