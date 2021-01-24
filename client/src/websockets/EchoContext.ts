import React from "react";
import Echo from 'laravel-echo';
const client = require('pusher-js');

interface EchoState {
    echo: Echo | null;
}

const EchoContext = React.createContext<EchoState>({
    echo:  null
});

export default EchoContext;