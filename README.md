# Home audio(WIP, not-functional)

## Goals

-   Search tracks independent of their location
-   Download tracks using a single interface from multiple providers(YouTube/Spotify/other?)
-   Control audio playback with low delay
-   Enable metadata using musicbrainz.org(optional)

## Status

Implemented:

-   Searching and downloading tracks from YouTube
-   Basic audio playback, track queuing

## Overview

-   [libhomeaudio](libhomeaudio/) - thin C++ wrapper for libpulse and ffmpeg using PHP-CPP
-   [client](client/) - React/TailwindCSS client
-   [app/routes](app/) - Lumen back-end with SQLite and laravel-websockets as a Pusher compatible broadcast mechanism
