<?php

namespace App\Observers;

use App\Events\TrackStatusChanged;
use App\Track;

class TrackObserver
{
    public function updating(Track $track)
    {
        if ($track->isDirty('status')) {
            event(new TrackStatusChanged($track, $track->getOriginal('status')));
        }
    }
}
