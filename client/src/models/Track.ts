export default interface Track {
    id: number;
    author: string | null;
    title: string;
    status: 'ready' | 'downloading' | 'download_failed';
}