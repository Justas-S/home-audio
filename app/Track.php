<?php

namespace App;

use Illuminate\Database\Eloquent\Model;

class Track extends Model
{
    const STATUS_DOWNLOADING = 'downloading';
    const STATUS_DOWNLOAD_FAILED = 'download_failed';
    const STATUS_DOWNLOADED = 'downloaded';
    const STATUS_TRANSCODING = 'transcoding';
    const STATUS_TRANSCODING_FAILED = 'transcoding_failed';
    const STATUS_READY = 'ready';

    protected $fillable = [
        'author', 'title', 'audio', 'source', 'source_id', 'status',
    ];
}
