<?php

namespace App\Jobs;

use App\Track;
use Exception;
use Illuminate\Support\Str;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Storage;
use FFmpegTranscoder;

class TranscodeTrack extends Job
{
    protected Track $track;

    public function __construct(Track $track)
    {
        $this->track = $track;
    }

    public function handle()
    {
        $this->transcode();
    }


    private function transcode()
    {
        $this->track->update(['status' => Track::STATUS_TRANSCODING]);
        $transcoder = new FFmpegTranscoder();

        $tempName = $this->track->id . '_' . Str::random(8) . '.mp3';
        $tempPath = sys_get_temp_dir() . '/' . $tempName;

        try {
            $transcoder->transcodeFile(Storage::path($this->track->audio), $tempPath, 0x15001);

            $newPath = 'audio/' . $this->track->id . '.mp3';
            // Storage::move($tempPath, $newPath);
            rename($tempPath, Storage::path($newPath));
            $this->track->update([
                'status' => Track::STATUS_READY,
                'audio' => $newPath
            ]);
        } catch (Exception $e) {
            $this->track->update(['status' => Track::STATUS_TRANSCODING_FAILED]);
            throw $e;
        } finally {
            @unlink($tempPath);
        }
    }
}
