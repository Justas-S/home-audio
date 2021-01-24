import { AxiosInstance } from "axios";
import Track from "../models/Track";

export default class TrackApi {

    private http: AxiosInstance;

    constructor(http: AxiosInstance) {
        this.http = http;
    }

    public async get(id: number): Promise<Track>
    {
        const resp = await this.http.get(`/track/${id}`);
        
        return resp.data;
    }

    public async download(id: number) {
        const resp = await this.http.post(`/track/${id}/download`);
        
        return resp.data;
    }

    public async queue(id: number) {
        const resp = await this.http.get(`/track/${id}/queue`);
        
        return resp.data;
    }

}