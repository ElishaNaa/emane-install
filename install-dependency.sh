#!/bin/bash

sudo apt-get install gcc g++ autoconf automake libtool libxml2-dev libprotobuf-dev \
python-protobuf libpcap-dev libpcre3-dev uuid-dev debhelper pkg-config \
python-setuptools protobuf-compiler git dh-python \
lxc bridge-utils mgen fping gpsd gpsd-clients \
iperf multitail olsrd openssh-server python-tk python-pmw python-lxml \
python-stdeb build-essential \
libboost-thread-dev libboost-system-dev libboost-test-dev libxml2 \
libxml2-dev libxml2-utils libreadline6-dev libprotobuf-dev \
protobuf-compiler doxygen graphviz cmake \
autotools-dev autoconf libtool uuid-dev libace-dev \
redis-server libperl-dev

sudo apt-get update
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"

sudo apt-get update
sudo apt-get install docker-ce=17.09.0~ce-0~ubuntu
sudo docker build -f emane-dlep-tutorial/docker/DockerSimulation -t simdoc .
