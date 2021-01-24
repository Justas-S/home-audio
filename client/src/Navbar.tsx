import React, { useState } from 'react';
import { useMutation, useQuery, useQueryCache } from 'react-query';
import AwesomeDebouncePromise from 'awesome-debounce-promise';
import Api from './Api';
import { ReactComponent as Search } from './heroicons/outline/search.svg';
import { ReactComponent as X } from './heroicons/outline/x.svg';
import { ReactComponent as Home } from './heroicons/outline/home.svg';
import Youtube from './icons/Youtube';


interface SearchResult {
  id: string;
  title: string;
  type: string;
}

const Navbar: React.FC = () => {
  const api = new Api();

  let [state, setState] = useState({
    currentIndex: -1,
    showResults: false,
  });

  const queryInput = React.createRef<HTMLInputElement>();
  const queryCache = useQueryCache();
  const { data: results } = useQuery<Array<SearchResult>>('search', { initialData: [] });

  const [mutate] = useMutation<Array<SearchResult>, Error, { q: string }>(async ({ q }) => {
    const results = await api.search(q);
    queryCache.setQueryData('search', results);
    setState({ ...state, showResults: true })
    return results;
  });

  const mutateDebounced = AwesomeDebouncePromise(text => {
    return mutate({ q: text });
  }, 300);

  async function handleInput(e: any) {
    const value = e.target.value;
    console.log('input', value);
    if (value.length < 2) {
      setState({ ...state, showResults: false })
      return;
    }

    mutateDebounced(value);
  }

  async function handleSelect(result: SearchResult) {

    api.queue(result.id, result.type).then(() => setState({ ...state, showResults: false }));

  }

  function handleReset() {
    setState({ ...state, showResults: false });
    queryInput.current!!.value = '';
  }

  function handleKeyDown(e: any) {
    console.log('key', e.key);

    if (e.key === 'ArrowUp' || e.key === 'ArrowDown') {
      let newIdx = state.currentIndex + (e.key === 'ArrowUp' ? -1 : 1);
      if (newIdx >= results!!.length) {
        newIdx = -1;
      } else if (newIdx < -1) {
        newIdx = results!!.length - 1;
      }
      setState({ ...state, currentIndex: newIdx });
    } else if (e.key === 'Enter') {
      if (state.currentIndex !== -1) {
        handleSelect(results!![state.currentIndex]);
      }
    }
  }

  return (
    <header className="flex items-center justify-center py-3 bg-gray-400">
      <div className="relative" onKeyDown={handleKeyDown}>
        <div className="relative">
          <input
            placeholder="Search..."
            onInput={handleInput}
            className="py-1 px-8 rounded"
            ref={queryInput}
            autoFocus
          />
          <Search className="absolute left-0 top-0 h-full py-2 pl-2"></Search>
          <X
            className="absolute right-0 top-0 h-full py-2 pr-2 cursor-pointer fill-current hover:text-black"
            onClick={handleReset}
          ></X>
        </div>
        {state.showResults && (
          <div className="absolute bg-gray-100 rounded w-full">
            <ul className="py-2">
              {results && results.map((result, idx) => (
                <li
                  key={result.id}
                  className={`p-3 mb-1 cursor-pointer hover:bg-gray-300 ${idx === state.currentIndex ? 'bg-gray-300' : ''
                    }`}
                >
                  <a onClick={() => handleSelect(result)}>
                    {result.type === 'youtube' && <span className="w-5 mr-2 inline-block"><Youtube></Youtube></span>}
                    {result.type === 'local' && <Home className="w-5 mr-2 inline-block stroke-current align-text-bottom"></Home>}
                    {result.title}
                  </a>
                </li>
              ))}
            </ul>
          </div>
        )}
      </div>
    </header>
  );
};
export default Navbar;
