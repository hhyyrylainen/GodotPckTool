FROM ubuntu:22.04 as builder

RUN apt-get update && apt-get -y upgrade && DEBIAN_FRONTEND=noninteractive apt-get -y install \
    make cmake build-essential g++ coreutils && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
