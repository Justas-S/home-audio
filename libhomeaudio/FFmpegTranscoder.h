#include <phpcpp.h>
#include <libavfilter/avfilter.h>

class FFmpegTranscoder : public Php::Base
{
private:
    AVFilterGraph *filter_graph;

public:
    void __construct();

    void transcodeFile(Php::Parameters &params);

    void __destruct();
};
