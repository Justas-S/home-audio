<?php

namespace App\Track;

use Illuminate\Support\Facades\Log;

class TrackSearchService
{

    protected $providers;

    public function __construct()
    {
        $this->providers = collect();
    }

    public function addProvider($provider)
    {
        $this->providers->put($provider->getName(), $provider);
    }

    public function search($query, $limit)
    {
        // TODO merge results in a smarter way
        // For example, local tracks should be prioritized when duplicate
        return $this->providers->flatMap(fn ($provider) => $provider->search($query, $limit / $this->providers->count()));
    }

    public function driver($name)
    {
        return $this->providers->get($name);
    }
}
