<?php

namespace App\Events;

use App\Track;
use Illuminate\Broadcasting\Channel;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;

class TrackQueued extends Event implements ShouldBroadcast
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
