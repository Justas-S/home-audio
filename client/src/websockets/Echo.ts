import Echo from 'laravel-echo';
const client = require('pusher-js');

const echo = new Echo({
    broadcaster: 'pusher',
    client: new client('ws-key', {
      wsHost: 'localhost',
      wsPort: 6001,
      forceTLS: false,
    }),
  });


export default echo as Echo;