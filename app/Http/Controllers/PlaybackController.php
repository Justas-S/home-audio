<?php

namespace App\Http\Controllers;

use App\Events\TrackPlaybackStarted;
use App\Jobs\PlayTrack;
use App\Track;
use App\Track\Player;
use App\Track\TrackSearchService;
use App\TrackQueue;
use Exception;
use Illuminate\Contracts\Bus\Dispatcher;
use Illuminate\Contracts\Cache\Repository;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Log;

class PlaybackController extends Controller
{

    public function __construct()
    {
        //
    }

    public function index(Player $player)
    {
        return $player->getCurrent();
    }

    public function play(Player $player)
    {
        $player->play();
    }

    public function pause(Player $player)
    {
        $player->pause();
    }

    public function next(Player $player)
    {
        return $player->next();
    }
}
