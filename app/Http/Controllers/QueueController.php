<?php

namespace App\Http\Controllers;

use App\Track;
use App\Track\TrackSearchService;
use App\TrackQueue;
use Illuminate\Http\Request;

class QueueController extends Controller
{

    public function __construct()
    {
        //
    }

    public function show(?string $name = 'default')
    {
        return TrackQueue::where('name', $name)->with('tracks')->first();
    }

    public function queue(Request $request, TrackSearchService $searcher)
    {
        $params = $this->validate($request, [
            'type' => 'required|max:255',
            'id' => 'required|max:255'
        ]);

        $track = Track::where('source_type', $params['type'])
            ->where('source_id', $params['id'])->first();

        if ($track) {
            return TrackQueue::queue($track);
        } else {
            $driver = $searcher->driver($params['type']);
            return $driver->queue($params['id']);
        }
    }
}
