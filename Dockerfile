FROM ubuntu:latest as executor

ENV TZ=Europe/Minsk
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && \
    apt-get -y install \
    cmake \
    g++ \
    libboost-all-dev \
    libssl-dev

WORKDIR /app

COPY . /app

RUN sh gen.sh && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

WORKDIR /app/build


ENTRYPOINT [ "/app/build/MyProject" ]