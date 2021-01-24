<?php

namespace App\Providers;

use App\Http\Controllers\TrackController;
use App\Observers\TrackObserver;
use App\Track;
use App\Track\SearchProvider\LocalSearchProvider;
use App\Track\SearchProvider\YouTubeSearchProvider;
use App\Track\TrackSearchService;
use Illuminate\Support\ServiceProvider;

class AppServiceProvider extends ServiceProvider
{
    /**
     * Register any application services.
     *
     * @return void
     */
    public function register()
    {
        $this->app->singleton(TrackSearchService::class, function ($app) {
            $service = new TrackSearchService();
            $service->addProvider($app->make(YouTubeSearchProvider::class));
            $service->addProvider($app->make(LocalSearchProvider::class));
            return $service;
        });


        // $service = new TrackSearchService();
        // $service->addProvider($this->app->make(YouTubeSearchProvider::class));
        // $this->app->singleton(TrackSearchService::class, $service);
    }

    public function boot()
    {
        Track::observe(TrackObserver::class);
    }
}
