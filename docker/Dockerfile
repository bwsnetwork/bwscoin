FROM ubuntu:20.04
MAINTAINER Constantin Yannuk <constantin.yannuk@upandrunningsoftware.com>

COPY *.deb /tmp/

RUN apt-get update -y -qq && \
    apt-get install -y wget libc6 libgcc1 libstdc++6 libzmq5 libevent-2.1-7 libevent-pthreads-2.1-7 libssl1.1 busybox \
    libboost-system1.71.0 libboost-filesystem1.71.0 libboost-chrono1.71.0 \
    libboost-program-options1.71.0 libboost-thread1.71.0  && \
    ldconfig && \
    dpkg -i /tmp/bwscoin*.deb

VOLUME /root/.bwscoin

EXPOSE 8566 8567 18566 18567 19566 19567

CMD bwscoind -txindex -printtoconsole -listenonion=0
