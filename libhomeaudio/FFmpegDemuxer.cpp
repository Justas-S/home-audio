#include <phpcpp.h>
#include <string>
#include <iostream>
#include <thread>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavutil/dict.h>
}
#include "FFmpegDemuxer.h"

Php::Array packet_to_array(AVPacket *packet);
Php::Array frame_to_array(AVFrame *frame, int bytes_per_sample);
static int receiveInternal(AVCodecContext *decoder, AVFrame *frame);
static int demuxAndDecodeInternal(AVFormatContext *demuxer, AVCodecContext *decoder, AVPacket *packet);

static void throw_av(int errnum, std::string prefix = "")
{
    char *buf = new char[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, buf, AV_ERROR_MAX_STRING_SIZE);
    throw Php::Exception("(" + std::to_string(errnum) + ")" + prefix + std::string(buf));
}

void FFmpegDemuxer::__construct(Php::Parameters &params)
{
    // av_log_set_level(AV_LOG_TRACE);
    const char *filename = params[0].rawValue();
    // const AVCodecID outputCodec = (AVCodecID)params[1].numericValue();

    demuxer = avformat_alloc_context();
    if (!demuxer)
    {
        throw new Php::Exception("avformat_alloc_context() failed");
    }

    // demuxer->skip_initial_bytes = 1105;
    demuxer->flags = AVFMT_FLAG_DISCARD_CORRUPT;
    int ret = 0;
    if ((ret = avformat_open_input(&demuxer, filename, NULL, NULL)) < 0)
    {
        throw_av(ret, "avformat_open_input failed: ");
    }

    // AVDictionary *options = NULL;
    if ((ret = avformat_find_stream_info(demuxer, NULL)) < 0)
    {
        throw_av(ret, "avformat_find_stream_info failed: ");
    }

    int stream_index = -1;
    for (int i = 0; i < demuxer->nb_streams; i++)
    {
        av_dump_format(demuxer, i, filename, 0);
        fprintf(stdout, "TEST: %d codec_id=%d\n", i, demuxer->streams[i]->codec->codec_id);
        if (demuxer->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if (stream_index == -1)
            {
                stream_index = i;
            }
        }
    }

    // packet.stream_index = stream_index; // ??
    // demuxer->streams[stream_index]->skip_samples = 1105;
    AVCodec *dec = avcodec_find_decoder(demuxer->streams[stream_index]->codec->codec_id);
    if (!dec)
    {
        throw new Php::Exception("avcodec_find_decoder() failed");
    }

    decoder = avcodec_alloc_context3(dec);
    if (!decoder)
    {
        throw new Php::Exception("avcodec_alloc_context3 failed");
    }

    if ((ret = avcodec_parameters_to_context(decoder, demuxer->streams[stream_index]->codecpar)) < 0)
    {
        throw_av(ret, "avcodec_parameters_to_context failed(): ");
    }

    ret = avcodec_open2(decoder, dec, NULL);
    if (ret < 0)
    {
        throw_av(ret, "avcodec_open2: ");
    }

    int i = 0;
    while (dec->sample_fmts != NULL)
    {
        const AVSampleFormat f = dec->sample_fmts[i++];
        if (f == AV_SAMPLE_FMT_NONE)
            break;
        fprintf(stdout, "sample format:%d\n", f);
    }
    // demuxer->streams[stream_index]->start_skip_samples = 1105;

    bytes_per_sample = av_get_bytes_per_sample(decoder->sample_fmt);
    fprintf(stdout, "stream_index: %d skip_samples:%d \n", stream_index, demuxer->streams[stream_index]->skip_samples);
    fprintf(stdout, "decoder. sample_fmt:%d byes per sample:%d codecname=%s\n", decoder->sample_fmt,
            av_get_bytes_per_sample(decoder->sample_fmt), decoder->codec->long_name);
    fprintf(stdout, "demuxer. time_base=%f flags=%d\n", av_q2d(demuxer->streams[stream_index]->time_base),
            demuxer->flags);

    AVCodec *enc = avcodec_find_decoder(AV_CODEC_ID_PCM_S16LE);
    if (!enc)
    {
        throw new Php::Exception("avcodec_find_decoder() failed");
    }

    encoder = avcodec_alloc_context3(enc);
    if (!encoder)
    {
        throw new Php::Exception("avcodec_alloc_context3 failed");
    }

    encoder->channels = 2;

    // enc_ctx->sample_rate = dec_ctx->sample_rate;
    // enc_ctx->channel_layout = dec_ctx->channel_layout;
    // enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
    // /* take first format from list of supported formats */
    // enc_ctx->sample_fmt = encoder->sample_fmts[0];
    // enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};

    ret = avcodec_open2(encoder, enc, NULL);
    if (ret < 0)
    {
        throw_av(ret, "avcodec_open2: ");
    }

    // Setup the filter graph

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
    {
        throw Php::Exception("avfilter_graph_alloc failed");
    }

    const AVFilter *buffersrc = avfilter_get_by_name("abuffer");
    if (!buffersrc)
    {
        avfilter_graph_free(&filter_graph);
        throw Php::Exception("avfilter_get_by_name(abuffer) failed");
    }

    AVFilterContext *bufferSrcCtx = NULL;
    char args[512];
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             decoder->time_base.num, decoder->time_base.den, decoder->sample_rate,
             av_get_sample_fmt_name(decoder->sample_fmt),
             decoder->channel_layout);
    fprintf(stdout, "args:%s\n", args);
    if ((ret = avfilter_graph_create_filter(&bufferSrcCtx, buffersrc, "buffer src filter", args, NULL, filter_graph)) < 0)
    {
        avfilter_graph_free(&filter_graph);
        throw_av(ret, "avfilter_graph_create_filter(buffersrc) failed: ");
    }
    fprintf(stdout, "buffersrc nb_inputs=%d\n", bufferSrcCtx->nb_inputs);

    const AVFilter *buffersink = avfilter_get_by_name("abuffersink");
    if (!buffersink)
    {
        avfilter_graph_free(&filter_graph);
        throw Php::Exception("avfilter_get_by_name(abuffersink) failed");
    }
    AVFilterContext *bufferSinkCtx = NULL;
    if ((ret = avfilter_graph_create_filter(&bufferSinkCtx, buffersink, "buffer sink filter", NULL, NULL, filter_graph)) < 0)
    {
        avfilter_graph_free(&filter_graph);
        throw_av(ret, "avfilter_graph_create_filter(buffersink) failed: ");
    }

    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *outputs = avfilter_inout_alloc();
    inputs->filter_ctx = bufferSinkCtx;
    inputs->name = av_strdup("out");
    inputs->next = NULL;
    inputs->pad_idx = 0;

    outputs->filter_ctx = bufferSrcCtx;
    outputs->name = av_strdup("in");
    if ((ret = avfilter_graph_parse_ptr(filter_graph, "anull",
                                        &inputs, &outputs, NULL)) < 0)
    {
        avfilter_graph_free(&filter_graph);
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw_av(ret, "avfilter_graph_parse_ptr failed: ");
    }

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
    {
        avfilter_graph_free(&filter_graph);
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        throw_av(ret, "avfilter_graph_config failed: ");
    }
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
}

void FFmpegDemuxer::__destruct()
{
    fprintf(stdout, "FFmpegDemuxer::__destruct\n");
    std::unique_lock<std::mutex> lk(demuxer_mutex);
    threaded_running = false;
    lk.unlock();
    demuxer_cv.notify_all();

    av_packet_unref(&packet);
    avfilter_graph_free(&filter_graph);
    avformat_free_context(demuxer);
    avcodec_free_context(&decoder);
    avcodec_free_context(&encoder);
}

Php::Value FFmpegDemuxer::demuxAndDecode()
{
    int ret = demuxAndDecodeInternal(demuxer, decoder, &packet);
    if (ret < 0)
    {
        if (ret == AVERROR_EOF)
            return Php::Value(false);
        if (ret == AVERROR(EAGAIN))
            return Php::Value(false);
        else
            throw_av(ret, "demuxAndDecodeInternal failed: ");
    }

    if (packet.stream_index != 0)
        return Php::Value(true);

    return Php::Value(true);
}

Php::Value FFmpegDemuxer::demuxDecodeAndFilter()
{
    int ret = 0;
    if ((ret = demuxAndDecodeInternal(demuxer, decoder, &packet)) < 0)
    {
        if (ret != AVERROR_EOF || ret != AVERROR(EAGAIN))
        {
            fprintf(stdout, "demuxDecodeAndFilter demuxer/decoder overflow: %d\n", ret);
            return Php::Value(false);
        }
        throw_av(ret, "demuxAndDecodeInternal failed: ");
    }
    AVFrame *frame = av_frame_alloc();
    if (!frame)
        throw_av(AVERROR(ENOMEM), "av_frame_alloc failed: ");

    if ((ret = receiveInternal(decoder, frame)) < 0)
    {
        av_frame_free(&frame);
        if (ret == AVERROR(EAGAIN))
        {
            fprintf(stdout, "demuxDecodeAndFilter decoder underflow: %d\n", ret);
            return Php::Value(false);
        }
        throw_av(ret, "receiveInternal failed: ");
    }

    int flags = AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT | AV_BUFFERSRC_FLAG_KEEP_REF;
    if ((ret = av_buffersrc_add_frame_flags(filter_graph->filters[0], frame, flags)) < 0)
    {
        av_frame_free(&frame);
        throw_av(ret, "av_buffersrc_write_frame failed: ");
    }
    av_frame_unref(frame);

    return Php::Value(true);
}

Php::Value FFmpegDemuxer::receiveFilter()
{
    std::unique_lock<std::mutex> lk(demuxer_mutex);

    AVFrame *frame = av_frame_alloc();
    int ret = 0;
    if (!frame)
    {
        threaded_error = ret;
        threaded_running = false;
        demuxer_cv.notify_one();
        throw_av(AVERROR(ENOMEM), "av_frame_alloc failed: ");
    }

    if ((ret = av_buffersink_get_frame(filter_graph->filters[filter_graph->nb_filters - 2], frame)) < 0)
    {
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        {
            fprintf(stdout, "receiveFilter buffer underflow: %d\n", ret);
            return Php::Value(false);
        }
        else
        {
            threaded_error = ret;
            threaded_running = false;
            demuxer_cv.notify_one();
            throw_av(ret, "av_buffersink_get_frame failed: ");
        }
    }

    Php::Array a = frame_to_array(frame, bytes_per_sample);
    av_frame_free(&frame);

    lk.unlock();
    demuxer_cv.notify_one();

    return a;
}

Php::Value FFmpegDemuxer::receive()
{

    AVFrame *frame = av_frame_alloc();
    if (!frame)
    {
        throw_av(AVERROR(ENOMEM), "av_frame_alloc failed: ");
    }

    int ret = receiveInternal(decoder, frame);
    if (ret < 0)
    {
        av_frame_free(&frame);
        if (ret == AVERROR(EAGAIN))
        {
            return Php::Value(false);
        }
        throw_av(ret, "receiveInternal failed: ");
    }

    Php::Array a;
    a = frame_to_array(frame, bytes_per_sample);
    // return packet_to_array(&packet);
    av_frame_free(&frame);

    return a;
}

Php::Value FFmpegDemuxer::beginDemuxDecodeAndFilter()
{
    if (this->isEof() || this->isError() || threaded_running)
        return Php::Value(false);

    threaded_running = true;
    // "this" is copied, might be issue
    std::thread(&FFmpegDemuxer::demuxDecodeAndFilterLoop, this).detach();

    return Php::Value(true);
}

Php::Value FFmpegDemuxer::isEof()
{
    return Php::Value(threaded_error == AVERROR_EOF);
}

Php::Value FFmpegDemuxer::isError()
{
    if (threaded_error == -1)
        return Php::Value(false);

    return Php::Value(threaded_error);
}

void FFmpegDemuxer::demuxDecodeAndFilterLoop()
{
    AVFrame *frame = av_frame_alloc();
    if (!frame)
        throw_av(AVERROR(ENOMEM), "av_frame_alloc failed: ");

    while (threaded_running)
    {
        fprintf(stdout, "demuxDecodeAndFilterLoop begins a loop\n");
        std::unique_lock<std::mutex> lk(demuxer_mutex);

        int ret = 0;
        if ((ret = demuxAndDecodeInternal(demuxer, decoder, &packet)) < 0)
        {
            if (ret == AVERROR(EAGAIN))
            {
                fprintf(stdout, "demuxDecodeAndFilterLoop demuxer/decoder overflow: %d\n", ret);
                demuxer_cv.wait(lk);
                if (!threaded_running)
                    break;
                continue;
            }

            if (ret == AVERROR_EOF)
                threaded_error = AVERROR_EOF;
            else
                threaded_error = ret;
            break;
        }

        if ((ret = receiveInternal(decoder, frame)) < 0)
        {
            if (ret == AVERROR(EAGAIN))
            {
                fprintf(stdout, "demuxDecodeAndFilterLoop decoder underflow: %d\n", ret);
                demuxer_cv.wait(lk);
                if (!threaded_running)
                    break;
                continue;
            }
            else
            {
                threaded_error = ret;
                break;
            }
        }

        int flags = AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT | AV_BUFFERSRC_FLAG_KEEP_REF;
        if ((ret = av_buffersrc_add_frame_flags(filter_graph->filters[0], frame, flags)) < 0)
        {
            // TODO check if EAGAIN??
            threaded_error = ret;
            break;
        }

        lk.unlock();
    }
    av_frame_unref(frame);
    threaded_running = false;
    fprintf(stdout, "demuxDecodeAndFilterLoop is done\n");
}

static int demuxAndDecodeInternal(AVFormatContext *demuxer, AVCodecContext *decoder, AVPacket *packet)
{
    // TODO flush
    std::cout << "demuxAndDecodeInternal" << std::endl;
    int ret = 0;

    if ((ret = av_read_frame(demuxer, packet)) < 0)
    {
        return ret;
    }

    // fprintf(stdout, "sidedata count:%d stream_index=%d\n", packet->side_data_elems, packet->stream_index);
    // for (int i = 0; i < packet->side_data_elems; i++)
    // {
    //     AVPacketSideData s = packet->side_data[i];
    //     fprintf(stdout, "type:%d\n", s.type);
    // }

    // fprintf(stdout, "decodere->timebase=%f encoder->time_base:%f\n",
    //         av_q2intfloat(decoder->time_base),
    //         av_q2intfloat(encoder->time_base));
    // av_packet_rescale_ts(&packet, decoder->time_base, encoder->time_base);

    if ((ret = avcodec_send_packet(decoder, packet)) < 0)
    {
        return ret;
    }

    return 0;
}

static int receiveInternal(AVCodecContext *decoder, AVFrame *frame)
{
    // std::cout << "receive" << std::endl;
    int ret = 0;
    if ((ret = avcodec_receive_frame(decoder, frame)) < 0)
    {
        return ret;
    }

    // fprintf(stdout, "open=%d encoder=%d encode_sub:%d encode2=%d receive_packet:%d\n", avcodec_is_open(encoder), av_codec_is_encoder(encoder->codec),
    //         encoder->codec->encode_sub,
    //         encoder->codec->encode2,
    //         encoder->codec->receive_packet);

    // if ((ret = avcodec_send_frame(encoder, frame)) < 0)
    // {
    //     av_frame_free(&frame);
    //     throw_av(ret, "avcodec_send_frame failed: ");
    // }

    // if ((ret = avcodec_receive_packet(encoder, &packet)) < 0)
    // {
    //     av_frame_free(&frame);
    //     throw_av(ret, "avcodec_receive_packet failed: ");
    // }
    return 0;
}

Php::Array packet_to_array(AVPacket *packet)
{
    Php::Array a;
    a["stream_index"] = packet->stream_index;
    a["convergence_duration"] = packet->convergence_duration;
    a["dts"] = packet->dts;
    a["duration"] = packet->duration;
    a["flags"] = packet->flags;
    a["pos"] = packet->pos;
    a["pts"] = packet->pts;
    a["size"] = packet->size;

    for (int i = 0; i < packet->buf->size; i++)
    {
        a["buf"][i] = packet->buf->data[i];
    }
    // packet->buf
    // packet->data
    // packet->side_data_elems
    // packet->side_data
    return a;
}
Php::Array frame_to_array(AVFrame *frame, int bytes_per_sample)
{

    Php::Array a;
    // a["best_effort_timestamp"] = frame->best_effort_timestamp;
    a["channels"] = frame->channels;
    // a["flags"] = frame->flags;
    a["format"] = frame->format;
    // a["interlaced_frame"] = frame->interlaced_frame;
    // a["key_frame"] = frame->key_frame;
    a["nb_samples"] = frame->nb_samples;
    // a["pkt_dts"] = frame->pkt_dts;
    // a["pkt_duration"] = frame->pkt_duration;
    // a["pkt_pos"] = frame->pkt_pos;
    // a["pkt_size"] = frame->pkt_size;
    // a["pts"] = frame->pts;
    // a["quality"] = frame->quality;
    a["sample_rate"] = frame->sample_rate;
    a["channel_layout"] = (int64_t)frame->channel_layout;

    uint64_t doffset = 0;
    uint64_t total_size = 0;

    total_size = frame->nb_samples * frame->channels * bytes_per_sample;
    char *buf = new char[total_size];
    // Interleave data bytes
    for (int i = 0; i < frame->nb_samples; i++)
    {
        for (int ch = 0; ch < frame->channels; ch++)
        {
            for (int k = 0; k < bytes_per_sample; k++)
            {
                buf[doffset++] = frame->data[ch][bytes_per_sample * i + k];
            }
        }
    }

    a["data"] = Php::Value(buf, total_size);

    AVDictionaryEntry *t = NULL;
    while (t = av_dict_get(frame->metadata, "", t, AV_DICT_IGNORE_SUFFIX))
    {
        a["metadata"][t->key] = t->value;
    }

    return a;
}