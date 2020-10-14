#include <phpcpp.h>
#include <pulse/simple.h>

class PulseAudioSimple : public Php::Base
{
private:
    pa_simple *s;

public:
    void __construct(Php::Parameters &params);
    void write(Php::Parameters &params);
    void drain();
    void __destruct();
};
