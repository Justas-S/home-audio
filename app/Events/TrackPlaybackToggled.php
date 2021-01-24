<?php

namespace App\Events;

use App\Track;
use Illuminate\Broadcasting\Channel;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;

class TrackPlaybackToggled extends Event implements ShouldBroadcast
{
    public Track $track;
    public bool $on;

    public function __construct(Track $track, bool $on)
    {
        $this->track = $track;
        $this->on = $on;
    }

    public function broadcastOn()
    {
        return new Channel('default');
    }
}
