import { AxiosInstance } from "axios";

export default class PlaybackApi {

    private http: AxiosInstance;

    constructor(http: AxiosInstance) {
        this.http = http;
    }


    public async get() {
        const resp = await this.http.get('/playback');

        return resp.data;
    }

    public async play() {
        const resp = await this.http.get('/playback/play');

        return resp.data;
    }

    public async pause() {
        const resp = await this.http.get('/playback/pause');

        return resp.data;
    }

    public async next() {
        const resp = await this.http.get('/playback/next');

        return resp.data;
    }
}

