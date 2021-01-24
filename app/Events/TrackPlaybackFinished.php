<?php

namespace App\Events;

use App\Track;
use Illuminate\Broadcasting\Channel;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;

/**
 * Fired when the track is played fully
 */
class TrackPlaybackFinished extends Event implements ShouldBroadcast
{
    public Track $track;

    public function __construct(Track $track)
    {
        $this->track = $track;
    }

    public function broadcastOn()
    {
        return new Channel('default');
    }
}
