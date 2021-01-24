import React, { useState, useEffect, ReactNode } from 'react';
import { ReactComponent as Play } from './../heroicons/outline/play.svg';
import { ReactComponent as Refresh } from './../heroicons/outline/refresh.svg';
import { ReactComponent as Logout } from './../heroicons/outline/logout.svg';
import Track from '../models/Track';
import Api from '../Api';
import { useQuery, useQueryCache } from 'react-query';
import { useEchoEvent } from '../websockets/EchoHooks';


const List: React.FC<{ tracks: Array<Track> }> = ({ tracks }) => {

  return (
    <section>

      {tracks.map((track) => {
        return <Item track={track}></Item>;
      })}

    </section>
  );
};

const Item: React.FC<{ track: Track }> = ({ track: initialTrack }) => {
  const api = new Api();

  const queryCache = useQueryCache();

  useEchoEvent('TrackStatusChanged', (args: any) => {
    console.log('TrackStatusChanged', args);
    queryCache.invalidateQueries(['tracks', args.track.id]);
  })

  const { data } = useQuery<Track>(['tracks', initialTrack.id], () => {
    return api.track.get(initialTrack.id);
  }, { initialData: initialTrack });

  const track = data as Track;


  async function restartDownload() {
    api.track.download(track.id);
  }

  async function play() {

  }

  async function queue() {
    api.track.queue(track.id);
  }

  return <div className="p-2 mb-1 flex flex-row">
    <div className="mb-2">

    </div>
    <div>
      <h5>{track.title}</h5>
      {track.author && <h6>{track.author}</h6>}

      {track.status !== 'ready' && <small className="font-bold">{track.status}
        {track.status === 'download_failed' && <Refresh title="Retry" onClick={restartDownload} className="h-4 ml-2 inline-block align-middle"></Refresh>}
      </small>}
      {track.status === 'ready' && <div className="flex align-items-center">
        <Play className="h-5" title="Play" onClick={play}></Play>
        <Logout className="h-5" title="Queue" onClick={queue}></Logout>
      </div>}
    </div>
  </div>
}

export {
  List,
  Item,
}
