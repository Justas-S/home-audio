import { useEffect } from "react";
import Echo from "./Echo";


interface Config {
    channel?: string;
    event: string;
}


export function useEchoEvent<TPayload>(listenFor: string | Config, callback: (params: TPayload) => any) {
    useEffect(() => {
        const channel = typeof listenFor === 'object' ? listenFor.channel || 'default' : 'default';
        const event = typeof listenFor === 'string' ? listenFor : listenFor.event;
        Echo.channel(channel).listen(event, callback);
        return () =>
          void Echo.channel(channel).stopListening(event);
      }, []);
}
