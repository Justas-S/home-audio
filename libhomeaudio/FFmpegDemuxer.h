#include <phpcpp.h>
#include <condition_variable>
#include <libavformat/avformat.h>
#include <libavcodec/codec.h>

class FFmpegDemuxer : public Php::Base
{
private:
    AVFormatContext *demuxer = NULL;
    AVCodecContext *decoder = NULL;
    AVCodecContext *encoder = NULL;
    AVFilterGraph *filter_graph = NULL;
    AVPacket packet;
    int bytes_per_sample = -1;

    bool threaded_running = false;
    int threaded_error = -1;
    std::condition_variable demuxer_cv;
    std::mutex demuxer_mutex;

    void demuxDecodeAndFilterLoop();

public:
    void __construct(Php::Parameters &params);

    // Basic, unbuffered
    Php::Value demuxAndDecode();
    Php::Value receive();

    // Buffered using a filter_graph as a buffer
    Php::Value demuxDecodeAndFilter();
    Php::Value receiveFilter();

    // Threaded and buffered using filter graph as a buffer
    Php::Value beginDemuxDecodeAndFilter();
    Php::Value isEof();
    Php::Value isError();

    void __destruct();
};
