import { ReactComponent as Refresh } from 'heroicons/solid/refresh.svg';
import React, { useState, useEffect } from 'react';
import { useQuery, useQueryCache } from 'react-query';
import Api from './Api';
import Queue from './models/Queue';
import Track from './models/Track';
import { Item } from './Track';
import { useEchoEvent } from './websockets/EchoHooks';


interface State {
  queue: Queue | null;
}

const TrackQueue: React.FC = () => {
  const api = new Api();

  const queryCache = useQueryCache();

  useEchoEvent<{ track: Track }>('TrackQueued', t => {
    console.log('TrackQueued');
    queryCache.invalidateQueries('trackQueue');
  });

  const { isLoading, isError, data: queue, error } = useQuery<Queue>('trackQueue', () => api.getQueue());

  if (isLoading) {
    return <Refresh className="h-5 animate-spin"></Refresh>
  }

  if (isError) {
    return <div>Queue failed to load: {(error as any).message}</div>
  }

  return (
    <section>
      {queue && (
        <div>
          {queue.tracks.map((track) => <Item track={track}></Item>)}
        </div>
      )}
    </section>
  );
};
export default TrackQueue;
