#include <phpcpp.h>
#include <stdlib.h>
#include "FFmpegTranscoder.h"
#include "FFmpegDemuxer.h"
#include "PulseAudioSimple.h"
#include "PulseAudioThreaded.h"

extern "C"
{

    /**
     *  Function that is called by PHP right after the PHP process
     *  has started, and that returns an address of an internal PHP
     *  strucure with all the details and features of your extension
     *
     *  @return void*   a pointer to an address that is understood by PHP
     */
    PHPCPP_EXPORT void *get_module()
    {

        // static(!) Php::Extension object that should stay in memory
        // for the entire duration of the process (that's why it's static)
        static Php::Extension extension("libhomeaudio", "0.0.1");

        Php::Class<FFmpegTranscoder> ffmpegTranscoder("FFmpegTranscoder");
        ffmpegTranscoder.method<&FFmpegTranscoder::__construct>("__construct");
        ffmpegTranscoder.method<&FFmpegTranscoder::transcodeFile>("transcodeFile");
        ffmpegTranscoder.method<&FFmpegTranscoder::__destruct>("__destruct");
        extension.add(std::move(ffmpegTranscoder));

        Php::Class<FFmpegDemuxer> ffmpedDemuxer("FFmpegDemuxer");
        ffmpedDemuxer.method<&FFmpegDemuxer::__construct>("__construct");
        ffmpedDemuxer.method<&FFmpegDemuxer::demuxAndDecode>("demuxAndDecode");
        ffmpedDemuxer.method<&FFmpegDemuxer::receive>("receive");
        ffmpedDemuxer.method<&FFmpegDemuxer::demuxDecodeAndFilter>("demuxDecodeAndFilter");
        ffmpedDemuxer.method<&FFmpegDemuxer::receiveFilter>("receiveFilter");
        ffmpedDemuxer.method<&FFmpegDemuxer::beginDemuxDecodeAndFilter>("beginDemuxDecodeAndFilter");
        ffmpedDemuxer.method<&FFmpegDemuxer::isEof>("isEof");
        ffmpedDemuxer.method<&FFmpegDemuxer::isError>("isError");
        ffmpedDemuxer.method<&FFmpegDemuxer::__destruct>("__destruct");

        ffmpedDemuxer.constant("AV_SAMPLE_FMT_NONE", AV_SAMPLE_FMT_NONE);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_U8", AV_SAMPLE_FMT_U8);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_S16", AV_SAMPLE_FMT_S16);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_S32", AV_SAMPLE_FMT_S32);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_FLT", AV_SAMPLE_FMT_FLT);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_DBL", AV_SAMPLE_FMT_DBL);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_U8P", AV_SAMPLE_FMT_U8P);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_S16P", AV_SAMPLE_FMT_S16P);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_S32P", AV_SAMPLE_FMT_S32P);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_FLTP", AV_SAMPLE_FMT_FLTP);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_DBLP", AV_SAMPLE_FMT_DBLP);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_S64", AV_SAMPLE_FMT_S64);
        ffmpedDemuxer.constant("AV_SAMPLE_FMT_S64P", AV_SAMPLE_FMT_S64P);
        extension.add(std::move(ffmpedDemuxer));

        Php::Class<PulseAudioSimple> paSimple("PulseAudioSimple");
        paSimple.method<&PulseAudioSimple::__construct>("__construct", {
                                                                           Php::ByVal("server", Php::Type::Null, false),
                                                                           Php::ByVal("name", Php::Type::String),
                                                                           Php::ByVal("direction", Php::Type::Numeric),
                                                                           Php::ByVal("dev", Php::Type::Null, false),
                                                                           Php::ByVal("streamName", Php::Type::String),
                                                                           Php::ByVal("sample", Php::Type::Array),
                                                                           Php::ByVal("channelMap", Php::Type::Array, false),
                                                                           Php::ByVal("bufferAttr", Php::Type::Array, false),
                                                                       });
        paSimple.method<&PulseAudioSimple::write>("write", {
                                                               //    Php::ByRef("data"),
                                                               //    Php::ByVal("len", Php::Type::Numeric),
                                                           });
        paSimple.method<&PulseAudioSimple::drain>("drain");
        paSimple.method<&PulseAudioSimple::__destruct>("__destruct");

        paSimple.constant("PA_STREAM_NODIRECTION", PA_STREAM_NODIRECTION);
        paSimple.constant("PA_STREAM_PLAYBACK", PA_STREAM_PLAYBACK);
        paSimple.constant("PA_STREAM_RECORD", PA_STREAM_RECORD);
        paSimple.constant("PA_STREAM_UPLOAD", PA_STREAM_UPLOAD);

        paSimple.constant("PA_SAMPLE_U8", PA_SAMPLE_U8);
        paSimple.constant("PA_SAMPLE_ALAW", PA_SAMPLE_ALAW);
        paSimple.constant("PA_SAMPLE_ULAW", PA_SAMPLE_ULAW);
        paSimple.constant("PA_SAMPLE_S16LE", PA_SAMPLE_S16LE);
        paSimple.constant("PA_SAMPLE_S16BE", PA_SAMPLE_S16BE);
        paSimple.constant("PA_SAMPLE_FLOAT32LE", PA_SAMPLE_FLOAT32LE);
        paSimple.constant("PA_SAMPLE_FLOAT32BE", PA_SAMPLE_FLOAT32BE);
        paSimple.constant("PA_SAMPLE_S32LE", PA_SAMPLE_S32LE);
        paSimple.constant("PA_SAMPLE_S32BE", PA_SAMPLE_S32BE);
        paSimple.constant("PA_SAMPLE_S24LE", PA_SAMPLE_S24LE);
        paSimple.constant("PA_SAMPLE_S24BE", PA_SAMPLE_S24BE);
        paSimple.constant("PA_SAMPLE_S24_32LE", PA_SAMPLE_S24_32LE);
        paSimple.constant("PA_SAMPLE_S24_32BE", PA_SAMPLE_S24_32BE);
        paSimple.constant("PA_SAMPLE_MAX", PA_SAMPLE_MAX);

        extension.add(std::move(paSimple));

        Php::Class<PulseAudioThreaded> paThreaded("PulseAudioThreaded");
        paThreaded.method<&PulseAudioThreaded::__construct>("__construct", {
                                                                               Php::ByVal("server", Php::Type::Null, false),
                                                                               Php::ByVal("name", Php::Type::String),
                                                                               Php::ByVal("direction", Php::Type::Numeric),
                                                                               Php::ByVal("dev", Php::Type::Null, false),
                                                                               Php::ByVal("streamName", Php::Type::String),
                                                                               Php::ByVal("sample", Php::Type::Array),
                                                                               Php::ByVal("channelMap", Php::Type::Array, false),
                                                                               Php::ByVal("bufferAttr", Php::Type::Array, false),
                                                                           });
        paThreaded.method<&PulseAudioThreaded::write>("write", {
                                                                   //    Php::ByRef("data"),
                                                                   //    Php::ByVal("len", Php::Type::Numeric),
                                                               });
        paThreaded.method<&PulseAudioThreaded::drain>("drain");
        paThreaded.method<&PulseAudioThreaded::__destruct>("__destruct");
        extension.add(std::move(paThreaded));

        return extension;
    }
}
