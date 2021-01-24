<?php

namespace App\Listeners;

use App\Events\TrackPlaybackFinished;
use App\Track\Player;
use App\TrackQueue;
use Illuminate\Contracts\Queue\ShouldQueue;
use Illuminate\Queue\InteractsWithQueue;

class PlayNextTrackInQueue
{
    private Player $player;

    public function __construct(Player $player)
    {
        $this->player = $player;
    }

    public function handle(TrackPlaybackFinished $event)
    {
        $queue = TrackQueue::getDefault();
        $nextTrack = $queue->next();
        if ($nextTrack->id !== $event->track->id) {
            return;
        }

        $queue->release($nextTrack);

        $nextTrack = $queue->next();
        if ($nextTrack) {
            $this->player->play($nextTrack);
        }
    }
}
