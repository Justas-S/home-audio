<?php

namespace App\Track\SearchProvider;

use App\Track;
use App\TrackQueue;

class LocalSearchProvider
{

    public function getName()
    {
        return 'local';
    }

    public function search($query, $limit)
    {
        return Track::where('status', Track::STATUS_READY)
            ->where(fn ($q) => $q
                ->where('title', 'LIKE', "%$query%")
                ->orWhere('author', 'LIKE', "%$query%"))
            ->limit($limit)
            ->get()
            ->map(fn ($track) => $this->toLocalTrack($track));
    }

    public function queue($id)
    {
        $track = Track::findOrFail($id);
        TrackQueue::queue($track);
    }

    private function toLocalTrack(Track $track)
    {
        return [
            'id' => $track->id,
            'type' => 'local',
            'title' => $track->title,
            'author' => $track->author,
        ];
    }
}
