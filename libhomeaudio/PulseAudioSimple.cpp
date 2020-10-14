#include <phpcpp.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <iostream>
#include "PulseAudioSimple.h"

void PulseAudioSimple::__construct(Php::Parameters &params)
{
    std::cout << "PulseAudioSimple::__construct:" << params.size() << std::endl;
    for (auto param : params)
    {
        std::cout << "Param:" << (param.isString() ? param.stringValue() : "") << " isNull:" << param.isNull() << " isArray:" << param.isArray() << std::endl;
        if (param.isArray())
        {
            int size = param.size();
            std::cout << "size: " << size << std::endl;

            for (auto el : param.mapValue())
            {
                std::cout << "element1: " << el.first << std::endl;
                std::cout << "element2: " << (el.second.stringValue()) << std::endl;
            }
        }
    }

    pa_stream_direction_t dir = PA_STREAM_NODIRECTION;
    pa_sample_spec spec = {};
    pa_channel_map *map = NULL;
    pa_buffer_attr *buffAttr = NULL;
    int error = 0;
    const char *server = NULL;
    if (!params[0].isNull())
        server = params[0].stringValue().c_str();

    const char *name = params[1].stringValue().c_str();
    const char *dev = params[3].isNull() ? NULL : params[3].stringValue().c_str();
    const char *streamName = params.size() > 4 && !params[4].isNull() ? params[4].stringValue().c_str() : NULL;
    std::cout << "streamname: " << streamName << " param: " << params[4].stringValue() << std::endl;

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

    if (!(s = ::pa_simple_new(server, name, dir, dev, streamName, &spec, map, buffAttr, &error)))
    {
        throw Php::Exception("Cannot create pulseaudio_simple: " + std::string(pa_strerror(error)));
    }

    std::cout << "PulseAudioSimple::__construct done" << error << std::endl;
}

void PulseAudioSimple::write(Php::Parameters &params)
{
    int error = 0;
    const char *data = params[0];
    size_t len = params[0].size();
    // std::cout << "PulseAudioSimple::write writing " << len << " bytes" << std::endl;

    if (pa_simple_write(s, data, len, &error) < 0)
    {
        throw Php::Exception("Cannot write to pulseaudio: " + std::string(pa_strerror(error)));
    }

    // if (pa_simple_drain(s, &error) < 0)
    // {
    //     throw Php::Exception("Cannot drain pulseaudio: " + std::string(pa_strerror(error)));
    // }
    // std::cout << "PulseAudioSimple::write drained" << error << std::endl;
}
void PulseAudioSimple::drain()
{
    int error = 0;
    if (pa_simple_drain(s, &error) < 0)
    {
        throw Php::Exception("Cannot drain pulseaudio: " + std::string(pa_strerror(error)));
    }
}

// void PulseAudioSimple::write(Php::Parameters &params)
// {
//     int error = 0;
//     const char *data = params[0];
//     size_t len = params[0].size();
//     std::cout << "PulseAudioSimple::write writing " << len << " bytes" << std::endl;

//     unsigned char out[32768];
//     size_t done = 0;
//     int ret = mpg123_decode(m, (unsigned char *)data, len, out, sizeof out, &done);
//     while (ret != MPG123_ERR && ret != MPG123_NEED_MORE)
//     {
//         if (ret == MPG123_OK)
//         {
//             if (done > 0 && pa_simple_write(s, out, done, &error) < 0)
//             {
//                 throw Php::Exception("Cannot write to pulseaudio: " + std::string(pa_strerror(error)));
//             }
//         }
//         else
//         {
//             std::cerr << "ret:" << ret << " done: " << done << std::endl;
//         }
//         ret = mpg123_decode(m, NULL, 0, out, sizeof out, &done);
//     }
//     if (ret == MPG123_ERR)
//     {
//         throw Php::Exception("Decoding failed: " + std::string(mpg123_strerror(m)));
//     }

//     if (pa_simple_drain(s, &error) < 0)
//     {
//         throw Php::Exception("Cannot drain pulseaudio: " + std::string(pa_strerror(error)));
//     }
//     std::cout << "PulseAudioSimple::write drained" << error << std::endl;
// }

void PulseAudioSimple::__destruct()
{
    std::cout << "PulseAudioSimple::__destruct()" << std::endl;
    pa_simple_free(s);
}