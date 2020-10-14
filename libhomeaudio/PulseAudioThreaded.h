#include <phpcpp.h>
#include <pulse/thread-mainloop.h>
#include <pulse/context.h>
#include <pulse/stream.h>

class PulseAudioThreaded : public Php::Base
{
private:
    pa_threaded_mainloop *loop;
    pa_context *ctx;
    pa_stream *stream;

    static void stream_state_callback(pa_stream *s, void *userdata);

public:
    void __construct(Php::Parameters &params);
    void write(Php::Parameters &params);
    void drain();
    void __destruct();
};
