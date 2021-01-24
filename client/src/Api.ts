import axios, { AxiosInstance } from 'axios';
import PlaybackApi from './api/PlaybackApi';
import TrackApi from './api/TrackApi';
export default class Api {

    private http: AxiosInstance;
    private _playback: PlaybackApi;
    private _track: TrackApi;

     get playback() {
        return this._playback;
    }

    get track() {
        return this._track;
    }

    constructor() {
        this.http = axios.create({
            baseURL: 'http://localhost:8000/api',
        });
        this._playback = new PlaybackApi(this.http);
        this._track = new TrackApi(this.http);
    }

    public async search(query: string): Promise<Array<any>> {
        const resp = await this.http.post('/track/search', {
            query,
        })

        return resp.data;
    }

    public async queue(id: string, type: string) {
        const resp = await this.http.post('/queue', {
            type, id
        })
        return resp.data;
    }

    public async getQueue() {
        const resp = await this.http.get('/queue');
        return resp.data;
    }

    public async getTracks() {
        const resp = await this.http.get('/track');
        return resp.data;
    }
    
}