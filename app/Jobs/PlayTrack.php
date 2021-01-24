<?php

namespace App\Jobs;

use App\Events\TrackPlaybackFinished;
use App\Events\TrackPlaybackStarted;
use App\Events\TrackPlaybackToggled;
use PulseAudioSimple;
use PulseAudioThreaded;
use FFmpegDemuxer;
use App\Track;
use Illuminate\Contracts\Cache\LockProvider;
use Illuminate\Contracts\Cache\Repository;
use Illuminate\Contracts\Events\Dispatcher;
use Illuminate\Contracts\Filesystem\Filesystem;
use Illuminate\Support\Facades\Cache;
use Illuminate\Support\Facades\Log;
use RuntimeException;
use Throwable;

class PlayTrack extends Job
{
    public int $tries = 1;
    public ?int $timeout = null;

    public $track;
    public $lockToken;
    /**
     * Create a new job instance.
     *
     * @return void
     */
    public function __construct(Track $track, $lockToken)
    {
        $this->track = $track;
        $this->lockToken = $lockToken;
    }

    /**
     * Execute the job.
     *
     * @return void
     */
    public function handle(Repository $cache, Filesystem $fs, Dispatcher $eventDisp)
    {
        $store = $cache->getStore();
        if (!($store instanceof LockProvider)) {
            throw new RuntimeException(sprintf(
                "Cache store must support '%s', instead '%s' given",
                LockProvider::class,
                get_class($store)
            ));
        }

        try {
            $cache->forever('playback_track', $this->track->id);
            $this->playTrack($cache, $fs, $eventDisp);

            $store->lock('playback_control')->get(function () use ($cache) {
                $cache->put('playback_stop', false);
            });
        } finally {
            $store->restoreLock('playback_player', $this->lockToken)->release();
            // TODO check if that is still the current track?
            $cache->forget('playback_track');
        }
    }

    public function failed(Throwable $exception)
    {
        event(new TrackPlaybackToggled($this->track, false));
        Cache::restoreLock('playback_player', $this->lockToken)->release();
        Cache::forget('playback_track');
    }

    protected function playTrack(Repository $cache, Filesystem $fs, Dispatcher $eventDisp)
    {
        $demuxer = new FFmpegDemuxer($fs->path($this->track->audio));
        $audioSink = null;
        $stopRequest = false;
        $counter = 0;

        $demuxer->beginDemuxDecodeAndFilter();
        do {
            if ($counter++ % 25 == 0 && $this->getStopRequest($cache)) {
                // TODO flush pulse
                $stopRequest = true;
                break;
            }
            $data = $demuxer->receiveFilter();
            if ($data === false) {
                break;
            }
            // TODO progress timestamps

            $string = $data['data'];
            if (strlen($string)) {
                if (!$audioSink) {
                    $audioSink = $this->initPulse($data);
                    $eventDisp->dispatch(new TrackPlaybackToggled($this->track, true));
                }
                $audioSink->write($string, strlen($string));
            }
            if ($demuxer->isError() !== false) {
                // If remuxing reached EOF, we can continue reading until filter EOF
                if ($demuxer->isEof()) {
                    continue;
                }
                break;
            }
        } while ($stopRequest === false);

        if (!$stopRequest) {
            $audioSink->drain();
            $eventDisp->dispatch(new TrackPlaybackFinished($this->track));
        }
        $eventDisp->dispatch(new TrackPlaybackToggled($this->track, false));
    }

    // protected function playTrack(Repository $cache, Filesystem $fs, Dispatcher $eventDisp)
    // {
    //     $demuxer = new FFmpegDemuxer($fs->path($this->track->audio));
    //     $audioSink = null;
    //     $sent = false;
    //     $stopRequest = false;

    //     $buffer = '';
    //     $counter = 0;

    //     // Buffering
    //     for ($i = 0; $i < 5 && $demuxer->demuxDecodeAndFilter(); $i++);

    //     do {
    //         if ($counter++ % 10 == 0 && $this->getStopRequest($cache)) {
    //             // TODO flush pulse
    //             break;
    //         }
    //         $data = $demuxer->receiveFilter();
    //         if ($data === false) {
    //             break;
    //         }
    //         // TODO progress timestamps

    //         // dump(array_diff_key($data, ["data" => '']));
    //         $string = $data['data'];
    //         if (strlen($string)) {
    //             $audioSink ??= $this->initPulse($data);

    //             if (!$sent) {
    //                 $eventDisp->dispatch(new TrackPlaybackToggled($this->track, true));
    //                 $sent = true;
    //             }

    //             $buffer .= $string;
    //             $audioSink->write($buffer, strlen($buffer));
    //             $buffer = '';
    //         }
    //     } while ($stopRequest === false && $demuxer->demuxDecodeAndFilter() !== false);

    //     if (!$stopRequest) {
    //         $audioSink->drain();
    //     } else {
    //         $eventDisp->dispatch(new TrackPlaybackToggled($this->track, false));
    //     }
    // }

    protected function getStopRequest($cache): bool
    {
        dump('performing stopRequest check');
        return $cache->lock('playback_control')->get(fn () => $cache->get('playback_stop'));
    }

    protected function initPulse(array $frame)
    {
        $format = $this->getPulseFormatFromAvFormat($frame['format']);
        $pulse = new PulseAudioThreaded(
            null,
            "Home-audio playback",
            PulseAudioSimple::PA_STREAM_PLAYBACK,
            null,
            "Home-audio stream: " . $this->track->title,
            [
                "rate" => $frame['sample_rate'],
                "format" => $format,
                "channels" => $frame['channels'],
            ]
        );
        return $pulse;
    }

    private function getPulseFormatFromAvFormat(int $avFormat)
    {
        // Whether PulseAudio values should be LE or BE is a guess
        switch ($avFormat) {
            case FFmpegDemuxer::AV_SAMPLE_FMT_U8:
                return PulseAudioSimple::PA_SAMPLE_U8;

            case FFmpegDemuxer::AV_SAMPLE_FMT_S16:
                return PulseAudioSimple::PA_SAMPLE_S16LE;

            case FFmpegDemuxer::AV_SAMPLE_FMT_S32:
                return PulseAudioSimple::PA_SAMPLE_S32LE;

            case FFmpegDemuxer::AV_SAMPLE_FMT_FLT:
            case FFmpegDemuxer::AV_SAMPLE_FMT_FLTP:
                return PulseAudioSimple::PA_SAMPLE_FLOAT32LE;

            default:
                throw new RuntimeException("Unexpected AV format: " . $avFormat);
        }
    }



    // public function handle(Repository $cache, Filesystem $fs, Dispatcher $eventDisp)
    // {
    //     try {
    //         $pulse = new PulseAudioSimple(
    //             null,
    //             "Home-audio playback",
    //             PulseAudioSimple::PA_STREAM_PLAYBACK,
    //             null,
    //             "Home-audio stream",
    //             [
    //                 "rate" => 44100,
    //                 "format" => PulseAudioSimple::PA_SAMPLE_S16LE,
    //                 "channels" => 2,
    //             ]
    //         );
    //         // $file = file_get_contents(storage_path('app/audio/test.mp3'));
    //         // TODO buffered reading
    //         $file = $fs->get($this->track->audio);
    //         $mpg123 = new Mpg123();
    //         $sent = false;

    //         $data = $mpg123->decode($file);
    //         while ($data) {
    //             $stopRequest = $cache->lock('playback_control')->get(fn () => $cache->get('playback_stop'));
    //             Log::debug('stop?' . $stopRequest);
    //             if ($stopRequest) {
    //                 // TODO flush pulse
    //                 break;
    //             }

    //             $pulse->write($data, strlen($data));
    //             if (!$sent) {
    //                 $eventDisp->dispatch(new TrackPlaybackStarted($this->track));
    //                 $sent = true;
    //             }
    //             $data = $mpg123->decode(null);
    //         }
    //         $pulse->drain();

    //         $cache->lock('playback_control')->get(function () use ($cache) {
    //             $cache->put('playback_stop', false);
    //         });
    //     } finally {
    //         $cache->restoreLock('playback_player', $this->lockToken)->release();
    //     }
    // }
}
