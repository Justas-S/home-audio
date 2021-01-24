<?php

namespace App\Events;

use App\Track;
use Illuminate\Broadcasting\Channel;
use Illuminate\Contracts\Broadcasting\ShouldBroadcast;

class TrackStatusChanged extends Event implements ShouldBroadcast
{
    public Track $track;
    public string $oldStatus;

    public function __construct(Track $track, string $oldStatus)
    {
        $this->track = $track;
        $this->oldStatus = $oldStatus;
    }

    public function broadcastOn()
    {
        return new Channel('default');
    }
}
