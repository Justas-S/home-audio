#include <phpcpp.h>
#include <string>
#include <iostream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}
#include "FFmpegTranscoder.h"

static int flush_encoder(AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx);
static int encode_write_frame(AVFrame *filt_frame, AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx);
static void filter_encode_write_frame(AVFrame *frame, AVFilterGraph *filter_graph, AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx);
static void init_filter(AVCodecContext *dec_ctx, AVCodecContext *enc_ctx,
                        const AVFilter *buffersrc,
                        const AVFilter *buffersink,
                        AVFilterGraph *filter_graph);

Php::Exception make_av_exception(int errnum, std::string prefix = "")
{
    char *buf = new char[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, buf, AV_ERROR_MAX_STRING_SIZE);
    return Php::Exception(std::to_string(errnum) + prefix + std::string(buf));
}

static void throw_av(int errnum, std::string prefix = "")
{
    throw make_av_exception(errnum, prefix);
}

void FFmpegTranscoder::__construct()
{
}

void FFmpegTranscoder::transcodeFile(Php::Parameters &params)
{
    const char *inputFilename = params[0].rawValue();
    const char *outputFilename = params[1].rawValue();
    AVCodecID outputCodec = (AVCodecID)params[2].numericValue();
    int ret;
    unsigned int i;
    AVFormatContext *ifmt_ctx = NULL;
    AVFormatContext *ofmt_ctx = NULL;

    std::cout << "input:" << (inputFilename == NULL ? "null" : inputFilename) << std::endl;
    std::cout << "output:" << (outputFilename == NULL ? "null" : outputFilename) << std::endl;

    if ((ret = avformat_open_input(&ifmt_ctx, inputFilename, NULL, NULL)) < 0)
    {
        throw_av(ret, "avformat_open_input:");
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
    {
        throw_av(ret, "avformat_find_stream_info:");
    }

    AVCodecContext *dec_ctx = NULL;

    int stream_index = -1;
    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream *stream = ifmt_ctx->streams[i];
        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!dec)
        {
            throw new Php::Exception("avcodec_find_decoder(" + std::to_string((int)stream->codecpar->codec_id) + ") failed for stream #" + std::to_string(i));
        }
        dec_ctx = avcodec_alloc_context3(dec);
        if (!dec_ctx)
        {
            throw new Php::Exception("avcodec_alloc_context3 failed for stream #" + std::to_string(i));
        }
        fprintf(stdout, "stream=%d type:%d\n", i, dec_ctx->codec_type);
        if (dec_ctx->codec_type != AVMEDIA_TYPE_AUDIO)
        {
            avcodec_free_context(&dec_ctx);
            dec_ctx = NULL;
            stream_index = -1;
            continue;
        }

        if ((avcodec_parameters_to_context(dec_ctx, stream->codecpar)) < 0)
        {
            avcodec_free_context(&dec_ctx);
            throw_av(ret, "avcodec_parameters_to_context: ");
        }

        if ((ret = ret = avcodec_open2(dec_ctx, dec, NULL)) < 0)
        {
            avcodec_free_context(&dec_ctx);
            throw_av(ret, "avcodec_open2: ");
        }
        stream_index = i;
        break;
    }
    fprintf(stdout, "input stream_index: %d dec_ctx:%d\n", stream_index, dec_ctx);
    av_dump_format(ifmt_ctx, stream_index, inputFilename, 0);

    if ((ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outputFilename)) < 0)
    {
        throw_av(ret, "avformat_alloc_output_context2 failed: ");
    }

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream)
    {
        throw new Php::Exception("avformat_new_stream failed");
    }

    /* in this example, we choose transcoding to same codec */
    // encoder = avcodec_find_encoder(dec_ctx->codec_id);
    AVCodec *encoder = avcodec_find_encoder(outputCodec);
    if (!encoder)
    {
        throw new Php::Exception("avcodec_find_encoder failed");
    }
    fprintf(stdout, "encoder: %d\n", encoder->id);

    AVCodecContext *enc_ctx = NULL;
    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx)
    {
        throw new Php::Exception("avcodec_alloc_context3 failed");
    }

    fprintf(stdout, "enc_ctx:%d dec_ctx:%d\n", enc_ctx, dec_ctx);
    enc_ctx->sample_rate = dec_ctx->sample_rate;
    enc_ctx->channel_layout = dec_ctx->channel_layout;
    enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
    // Set some defaults for unknown formats
    if (enc_ctx->channels == 0)
        enc_ctx->channels = 2;
    if (enc_ctx->channel_layout == 0)
        enc_ctx->channel_layout = AV_CH_LAYOUT_STEREO;

    /* take first format from list of supported formats */
    enc_ctx->sample_fmt = encoder->sample_fmts[0];
    enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* Third parameter can be used to pass settings to encoder */
    ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0)
    {
        throw_av(ret, "avcodec_open2:");
    }
    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0)
    {
        throw_av(ret, "avcodec_parameters_from_context:");
    }
    out_stream->time_base = enc_ctx->time_base;

    av_dump_format(ofmt_ctx, 0, outputFilename, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, outputFilename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            throw_av(ret, "avio_open: ");
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        throw_av(ret, "avformat_write_header: ");
    }

    const AVFilter *buffersrc = NULL;
    const AVFilter *buffersink = NULL;
    buffersrc = avfilter_get_by_name("abuffer");
    buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersrc || !buffersink)
        throw_av(AVERROR_UNKNOWN, "avfilter_get_by_name failed");

    AVFilterGraph *filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
        throw_av(AVERROR_UNKNOWN, "avfilter_graph_alloc failed");

    try
    {
        init_filter(dec_ctx, enc_ctx, buffersrc, buffersink, filter_graph);
    }
    catch (Php::Exception e)
    {
        avfilter_graph_free(&filter_graph);

        throw e;
    }

    //
    AVPacket packet = {buf : NULL, pts : NULL, dts : NULL, data : NULL, size : 0};
    AVFrame *frame = NULL;
    // int got_frame;
    /* read all packets */
    while (1)
    {
        ifmt_ctx->skip_initial_bytes = 1150;
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;

        frame = av_frame_alloc();
        if (!frame)
        {
            ret = AVERROR(ENOMEM);
            break;
        }
        av_packet_rescale_ts(&packet,
                             ifmt_ctx->streams[0]->time_base,
                             dec_ctx->time_base);
        // ret = avcodec_decode_audio4(dec_ctx, frame, &got_frame, &packet);
        // if (ret < 0)
        // {
        //     throw_av(ret, "avcodec_decode_audio4 failed: ");
        // }
        if ((ret = avcodec_send_packet(dec_ctx, &packet)) < 0)
        {
            av_frame_free(&frame);
            throw_av(ret, "avcodec_send_packet failed: ");
        }
        if ((ret = avcodec_receive_frame(dec_ctx, frame)) < 0)
        {
            av_frame_free(&frame);
            throw_av(ret, "avcodec_receive_frame failed: ");
        }

        frame->pts = frame->best_effort_timestamp;
        try
        {
            filter_encode_write_frame(frame, filter_graph, enc_ctx, ofmt_ctx);
            av_frame_free(&frame);
        }
        catch (Php::Exception e)
        {
            av_frame_free(&frame);
            throw e;
        }

        av_packet_unref(&packet);
    }

    /* flush filters and encoders */
    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        filter_encode_write_frame(NULL, filter_graph, enc_ctx, ofmt_ctx);
        /* flush encoder */
        flush_encoder(enc_ctx, ofmt_ctx);
    }
    av_write_trailer(ofmt_ctx);
}

void FFmpegTranscoder::__destruct()
{
    //
}

static int flush_encoder(AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx)
{
    int ret;
    if (!(enc_ctx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;
    while (encode_write_frame(NULL, enc_ctx, ofmt_ctx))
        ;
    return ret;
}

static void filter_encode_write_frame(AVFrame *frame, AVFilterGraph *filter_graph, AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx)
{
    int ret;
    AVFrame *filt_frame;

    /* push the decoded frame into the filtergraph */
    if ((ret = av_buffersrc_add_frame_flags(filter_graph->filters[0], frame, 0)) < 0)
    {
        throw_av(ret, "av_buffersrc_add_frame_flags: ");
    }
    /* pull filtered frames from the filtergraph */
    while (1)
    {
        filt_frame = av_frame_alloc();
        if (!filt_frame)
        {
            throw Php::Exception("cannot allocate av_frame");
        }

        if ((ret = av_buffersink_get_frame(filter_graph->filters[1], filt_frame)) < 0)
        {
            av_frame_free(&filt_frame);
            /* if no more frames for output - returns AVERROR(EAGAIN)
             * if flushed and no more frames for output - returns AVERROR_EOF
             * rewrite retcode to 0 to show it as normal procedure completion
             */
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                return;
            }
            else
            {
                throw_av(ret, "av_buffersink_get_frame failed: ");
            }
        }
        filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
        if (!encode_write_frame(filt_frame, enc_ctx, ofmt_ctx))
        {
            break;
        }
    }
}

static int encode_write_frame(AVFrame *filt_frame, AVCodecContext *enc_ctx, AVFormatContext *ofmt_ctx)
{
    int ret;
    int got_frame;
    AVPacket enc_pkt;

    /* encode filtered frame */
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = avcodec_encode_audio2(enc_ctx, &enc_pkt,
                                filt_frame, &got_frame);
    av_frame_free(&filt_frame);
    if (ret < 0)
        throw_av(ret, "avcodec_encode_audio2 failed:");
    if (!got_frame)
        return 0;

    /* prepare packet for muxing */
    enc_pkt.stream_index = 0;
    av_packet_rescale_ts(&enc_pkt,
                         enc_ctx->time_base,
                         ofmt_ctx->streams[0]->time_base);
    /* mux encoded frame */
    if ((ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt)) < 0)
        throw_av(ret, "av_interleaved_write_frame failed: ");

    return 1;
}

static void init_filter(AVCodecContext *dec_ctx, AVCodecContext *enc_ctx,
                        const AVFilter *buffersrc,
                        const AVFilter *buffersink,
                        AVFilterGraph *filter_graph)
{

    if (dec_ctx->codec_type != AVMEDIA_TYPE_AUDIO)
        throw_av(AVERROR_UNKNOWN, "unknown codec_type");

    int ret = 0;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    char args[512];
    AVFilterContext *buffersrc_ctx = NULL;
    AVFilterContext *buffersink_ctx = NULL;
    Php::Exception *ex = NULL;

    if (!outputs || !inputs)
    {
        throw_av(AVERROR_UNKNOWN, "avfilter_inout_alloc failed:");
    }

    if (!dec_ctx->channel_layout)
        dec_ctx->channel_layout =
            av_get_default_channel_layout(dec_ctx->channels);

    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
             av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    if (ret < 0)
    {
        throw_av(ret, "avfilter_graph_create_filter: ");
    }
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0)
    {
        throw_av(ret, "avfilter_graph_create_filter: ");
    }
    ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
                         (uint8_t *)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        throw_av(ret, "av_opt_set_bin: ");
    }

    ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
                         (uint8_t *)&enc_ctx->channel_layout,
                         sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        throw_av(AVERROR_UNKNOWN, "Cannot set output channel layout\n");
    }
    ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
                         (uint8_t *)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        throw_av(AVERROR_UNKNOWN, "Cannot set output sample rate\n");
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    if (!outputs->name || !inputs->name)
    {
        throw_av(AVERROR_UNKNOWN, "av_strdup failed: ");
    }
    if ((ret = avfilter_graph_parse_ptr(filter_graph, "anull", &inputs, &outputs, NULL)) < 0)
    {
        throw_av(ret, "avfilter_graph_parse_ptr failed: ");
    }
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
    {
        throw_av(ret, "avfilter_graph_config failed: ");
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    if (ex)
        throw ex;
}