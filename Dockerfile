FROM ubuntu:latest

RUN apt-get -y update
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install tzdata
ENV LANG en_US.utf8

RUN apt-get install -y build-essential zlib1g \
    zlib1g-dev cmake libgtest-dev ninja-build vim \
    libpcap-dev  libssl-dev libboost-all-dev \
    clang-tidy libhiredis-dev

RUN apt install -y python3-pip
RUN pip install grequests

RUN mkdir /workspace
