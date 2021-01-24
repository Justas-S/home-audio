<?php

/*
|--------------------------------------------------------------------------
| Application Routes
|--------------------------------------------------------------------------
|
| Here is where you can register all of the routes for an application.
| It is a breeze. Simply tell Lumen the URIs it should respond to
| and give it the Closure to call when that URI is requested.
|
*/

use Illuminate\Support\Facades\Route;

$router->get('/', function () use ($router) {
    return $router->app->version();
});


$router->group(['prefix' => '/api'], function () use ($router) {
    $router->group(['prefix' => '/track'], function () use ($router) {
        $router->get('/', 'TrackController@index');
        $router->post('/search', 'TrackController@search');

        $router->group(['prefix' => '/{id}'], function ()  use ($router) {
            $router->get('/', 'TrackController@show');
            $router->post('/download', 'TrackController@download');
            $router->get('/queue', 'TrackController@queue');
        });
    });

    $router->get('/queue[/name]', 'QueueController@show');
    $router->post('/queue', 'QueueController@queue');

    $router->group(['prefix' => '/playback'], function () use ($router) {
        $router->get('/', 'PlaybackController@index');
        $router->get('/play', 'PlaybackController@play');
        $router->get('/pause', 'PlaybackController@pause');
        $router->get('/next', 'PlaybackController@next');
    });
});
