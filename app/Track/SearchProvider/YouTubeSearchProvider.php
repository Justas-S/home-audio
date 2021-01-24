<?php

namespace App\Track\SearchProvider;

use App\Jobs\DownloadYouTubeVideo;
use App\Jobs\TranscodeTrack;
use App\Track;
use App\Track\TrackSearchResult;
use App\TrackQueue;
use Google_Client;
use Google_Service_YouTube;
use Google_Service_YouTube_SearchResult;
use Illuminate\Support\Facades\Log;
use YouTube\YouTubeDownloader;

class YouTubeSearchProvider
{
    protected Google_Client $client;
    protected YouTubeDownloader $downloader;

    public function __construct()
    {
        $this->client = new Google_Client();
        $this->client->setDeveloperKey(env('GOOGLE_DEVELOPER_KEY'));

        $this->downloader = new YouTubeDownloader();
    }

    public function getName()
    {
        return 'youtube';
    }

    public function search($query, $limit)
    {
        $ytClient = new Google_Service_YouTube($this->client);
        $results = $ytClient->search->listSearch('snippet,id', [
            'q' => $query,
            'maxResults' => $limit
        ]);

        $tracks = collect();
        while ($results->next()) {
            /** @var Google_Service_YouTube_SearchResult */
            $result = $results->current();
            $snippet = $result->getSnippet();
            if (!$result->getId()->videoId) {
                continue;
            }

            $track = new TrackSearchResult();
            $track->type = $this->getName();
            $track->title = $snippet->getTitle();
            $track->id = $result->getId()->videoId;
            $tracks->push($track);
        }
        return $tracks;
    }

    public function queue($id)
    {
        $track = Track::where('source', 'youtube')
            ->where('source_id', $id)
            ->first();

        if ($track) {
            switch ($track->status) {
                case Track::STATUS_TRANSCODING_FAILED:
                    dispatch(new TranscodeTrack($track));
                case Track::STATUS_READY:
                case Track::STATUS_DOWNLOADING:
                case Track::STATUS_TRANSCODING:
                    return;
            }
        } else {
            $ytClient = new Google_Service_YouTube($this->client);
            $results = $ytClient->videos->listVideos('snippet', [
                'id' => $id
            ]);
            /** @var Google_Service_YouTube_SearchResult */
            $result = $results->count() ? $results->current() : null;
            if (!$result) {
                Log::warning('no results for ' . $id);
                return false;
            }
            $title = $result->getSnippet()->title;
            // TODO attempt to extract author?
            $track = Track::create([
                'author' => '',
                'title' => $title,
                'source' => 'youtube',
                'source_id' => $id,
                'status' => 'created',
            ]);
        }

        TrackQueue::queue($track);

        $links = $this->downloader->getDownloadLinks('https://www.youtube.com/watch?v=' . $id);
        Log::debug($links);
        $link  = collect($links)->first(fn ($link) => strpos($link['format'], 'audio') !== false &&
            strpos($link['format'], 'video') === false &&
            strpos($link['format'], 'webm') !== false);
        if (!$link) {
            return false;
        }



        dispatch(new DownloadYouTubeVideo($track, $link['url'], trim(explode(",", $link['format'])[0])));
    }
}
