<?php

namespace App\Jobs;

use App\Track;
use App\TrackQueue;
use Exception;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Storage;

class DownloadYouTubeVideo extends Job
{
    protected Track $track;
    protected string $url;
    protected string $fileFormat;

    public function __construct(Track $track, $url, $format)
    {
        $this->track = $track;
        $this->url = $url;
        $this->fileFormat = $format;
    }

    public function handle()
    {
        $this->download();

        if ($this->fileFormat !== 'mp3') {
            dispatch(new TranscodeTrack($this->track));
        }
    }

    public function progress($resource, $download_size, $downloaded)
    {
        Log::debug('progress ' . $downloaded . ' of ' . $download_size);
        // TODO send event
    }

    private function download()
    {
        $this->track->update(['status' => Track::STATUS_DOWNLOADING]);

        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $this->url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_PROGRESSFUNCTION, [$this, 'progress']);
        curl_setopt($ch, CURLOPT_NOPROGRESS, false);
        curl_setopt($ch, CURLOPT_HEADER, 0);
        $audio = curl_exec($ch);
        $errno = curl_errno($ch);

        if ($errno !== 0) {
            $this->track->update(['status' => Track::STATUS_DOWNLOAD_FAILED]);
            Log::error("Track #{$this->track->id} error: " . curl_error($ch));
            curl_close($ch);
            return;
        }
        curl_close($ch);

        try {
            $path = 'audio/' . $this->track->id . '.' . $this->fileFormat;
            Storage::put($path, $audio);
            $this->track->update([
                'audio' => $path,
                'status' => Track::STATUS_DOWNLOADED
            ]);
        } catch (Exception $e) {
            $this->track->update(['status' => Track::STATUS_DOWNLOAD_FAILED]);
            throw $e;
        }
    }
}
