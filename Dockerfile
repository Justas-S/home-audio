

# make install

RUN pip3 install pyspotify

RUN  curl -L https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer | bash

RUN apt-get update && apt-get install lame build-essential libffi-dev

#RUN wget https://developer.spotify.com/download/libspotify/libspotify-12.1.51-Linux-x86_64-release.tar.gz \
#    tar xvf libspotify-12.1.51-Linux-x86_64-release.tar.gz

RUN git clone https://github.com/hbashton/spotify-ripper.git && cd spotify-ripper && python setup.py install

