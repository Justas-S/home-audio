<?php

namespace App\Http\Controllers;

use App\Track;
use App\Track\Player;
use App\Track\TrackSearchService;
use App\TrackQueue;
use Illuminate\Http\Request;

class TrackController extends Controller
{

    public function __construct()
    {
        //
    }

    public function index()
    {
        return Track::get();
    }

    public function show(string $id)
    {
        return Track::findOrFail($id);
    }

    public function download(string $id, TrackSearchService $searcher)
    {
        $track = Track::findOrFail($id);
        $searcher->driver($track->source)->queue($track->source_id);
    }

    public function queue(string $id, Player $player)
    {
        $track = Track::findOrFail($id);
        TrackQueue::queue($track);
        if (!$player->isPlaying()) {
            $player->play();
        }
    }

    public function search(Request $request, TrackSearchService $searcher)
    {
        $query = $this->validate($request, [
            'query' => 'required|min:2|max:255',
        ])['query'];

        return $searcher->search($query, 10);
    }
}
