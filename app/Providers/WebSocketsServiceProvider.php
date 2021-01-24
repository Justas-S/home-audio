<?php

namespace App\Providers;


use BeyondCode\LaravelWebSockets\WebSocketsServiceProvider as BaseWebSocketsServiceProvider;


class WebSocketsServiceProvider extends BaseWebSocketsServiceProvider
{
    protected function registerRoutes()
    {
        return $this;
    }
}
