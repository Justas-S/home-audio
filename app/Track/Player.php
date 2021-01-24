<?php

namespace App\Track;

use App\Jobs\PlayTrack;
use App\Track;
use App\TrackQueue;
use Exception;
use Illuminate\Contracts\Bus\Dispatcher;
use Illuminate\Contracts\Cache\Repository;
use Illuminate\Support\Facades\Log;

class Player
{
    protected Repository $cache;
    protected Dispatcher $dispatcher;

    public function __construct(Repository $cache, Dispatcher $dispatcher)
    {
        $this->cache = $cache;
        $this->dispatcher = $dispatcher;
    }

    public function getCurrent(): array
    {
        $track = $this->cache->get('playback_track');
        return [
            'track' => Track::find($track),
        ];
    }

    public function isPlaying(): bool
    {
        return !!$this->cache->get('playback_track');
    }

    public function play(Track $track = null)
    {
        $this->togglePlaybackStop(true);
        $track ??= TrackQueue::getDefault()->next();

        if (!$track) return;

        $lock = $this->cache->lock('playback_player');
        try {
            $lock->block(3);

            $this->togglePlaybackStop(false);

            $job = new PlayTrack($track, $lock->owner());
            $this->dispatcher->dispatch($job);
        } catch (Exception $e) {
            optional($lock)->release();
            throw $e;
        }
    }

    public function pause()
    {
        $this->togglePlaybackStop(true);

        $lock = $this->cache->lock('playback_player');
        try {
            $lock->block(3);
            Log::debug('pause() got the player lock');

            $this->togglePlaybackStop(false);
        } finally {
            optional($lock)->release();
        }
    }

    public function next()
    {
        $queue = TrackQueue::getDefault();

        if ($this->isPlaying()) {
            $queue->release($this->getCurrent()['track']);
        }

        $nextTrack = $queue->next();
        $this->play($nextTrack);
        return $nextTrack;
    }

    protected function togglePlaybackStop($toggle)
    {
        $this->cache->lock('playback_control')->get(function () use ($toggle) {
            $this->cache->put('playback_stop', $toggle);
        });
    }
}
