<?php

namespace App;

use App\Events\TrackQueued;
use Illuminate\Database\Eloquent\Model;

class TrackQueue extends Model
{
    protected $fillable = [
        'track_id', 'name',
    ];

    /**
     * TODO proper ordering
     */
    public function tracks()
    {
        return $this->belongsToMany(Track::class, 'track_queue_track'); //->orderByPivot('id', 'desc');
    }

    public static function queue(Track $track, $queueName = 'default')
    {
        /** @var static $queue */
        $queue = static::query()->firstOrCreate(['name' => $queueName], ['name' => $queueName]);

        $queue->tracks()->attach($track->id);

        event(new TrackQueued($track));
    }

    public function next(): ?Track
    {
        return $this->tracks()->first();
    }

    public function release(Track $track)
    {
        return $this->tracks()->detach($track->id);
    }

    public static function getDefault(): ?TrackQueue
    {
        return static::where('name', 'default')->first();
    }
}
