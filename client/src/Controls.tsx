import React, { useState, useEffect } from 'react';
import Api from './Api';
import { ReactComponent as Play } from './heroicons/outline/play.svg';
import { ReactComponent as Pause } from './heroicons/outline/pause.svg';
import { ReactComponent as Next } from './heroicons/outline/chevron-right.svg';
import Track from './models/Track';
import Echo from './websockets/Echo';
import { useEchoEvent } from './websockets/EchoHooks';

interface State {
  currentTrack: Track | null;
  isPlaying: boolean;
  loading: boolean;
}

const Controls: React.FC = () => {
  const api = new Api();

  useEchoEvent<{ on: boolean }>('TrackPlaybackToggled', ({ on }) => {
    console.log('TrackPlaybackToggled', on);
    setState({ ...state, isPlaying: on, loading: false });
  })

  const [state, setState] = useState<State>({
    currentTrack: null,
    isPlaying: false,
    loading: false,
  });

  useEffect(() => {
    (async () => {
      setState({ ...state, loading: true });
      try {
        const playback = await api.playback.get();
        console.log('getting playback');
        setState({
          ...state,
          currentTrack: playback.track,
          isPlaying: !!playback.track,
          loading: false,
        });
      } catch (e) {
        setState({ ...state, loading: false });
        throw e;
      }
    })();
  }, []);

  async function handleClickPlay() {
    togglePlayback(true);
  }

  async function handleClickPause() {
    togglePlayback(false);
  }

  async function handleClickNext() {
    setState({ ...state, loading: true });
    try {
      await api.playback.next();
    } catch (e) {
      setState({ ...state, loading: false });
      throw e;
    }
  }

  async function togglePlayback(play: boolean) {
    setState({ ...state, loading: true });
    try {
      play ? await api.playback.play() : await api.playback.pause();
      setTimeout(() => {
        setState((prevState) => ({ ...prevState, loading: false }));
      }, 2000);
    } catch (e) {
      setState({ ...state, loading: false });
      throw e;
    }
  }

  return (
    <footer className="fixed flex justify-center align-center bottom-0 left-0 right-0 bg-gray-400 py-2 px-4">
      <div>
        <button
          className="align-middle hover:text-gray-200 disabled:opacity-75"
          disabled={state.loading}
        >
          {state.isPlaying ? (
            <Pause
              className="h-12 stroke-current"
              onClick={handleClickPause}
            ></Pause>
          ) : (
              <Play
                className="h-12 stroke-current"
                onClick={handleClickPlay}
              ></Play>
            )}
        </button>
        <button className="align-middle hover:text-gray-200" disabled={state.loading}>
          <Next className="h-12" onClick={handleClickNext}></Next>
        </button>
      </div>
    </footer>
  );
};
export default Controls;
