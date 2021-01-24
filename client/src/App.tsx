import React, { useEffect, useState } from 'react';
import './App.css';
import Navbar from './Navbar';
import TrackQueue from './TrackQueue';
import Controls from './Controls';
import Api from './Api';
import Track from './models/Track';
import { List as TrackList } from './Track';
import { ReactQueryDevtools } from 'react-query-devtools';
import { QueryCache, ReactQueryCacheProvider } from 'react-query';


const queryCache = new QueryCache({
  defaultConfig: {
    queries: { refetchOnWindowFocus: false },
  }
})

const App: React.FunctionComponent = () => {

  const api = new Api();
  const [state, setState] = useState<Array<Track>>([]);

  useEffect(() => {
    (async () => {
      const tracks = await api.getTracks();
      setState(tracks);
    })();
  }, []);

  return (
    <main className="">
      <ReactQueryCacheProvider queryCache={queryCache}>
        <Navbar></Navbar>
        <div className="container mx-auto mt-4 pb-16 flex">
          <section>
            <h1 className="text-2xl text-black">Current queue</h1>
            <TrackQueue></TrackQueue>
          </section>
          <TrackList tracks={state}></TrackList>
        </div>
        <Controls></Controls>
        <ReactQueryDevtools />
      </ReactQueryCacheProvider>
    </main>
  );
};

export default App;
