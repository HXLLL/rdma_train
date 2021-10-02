#!/bin/bash

if [[ `whoami` != root ]]; then
    echo "Please run as root"
    exit 0
fi

need_manual=

echo "Installing libyaml"

wget http://pyyaml.org/download/libyaml/yaml-0.2.5.tar.gz

if [[ -e yaml-0.2.5.tar.gz ]]; then
    tar xvf yaml-0.2.5.tar.gz
    cd yaml-0.2.5.tar.gz
    ./configure
    make
    make install
    cd ..
else
    echo "ERROR: Can't download libyaml, please install it manually."
    need_manual=$need_manual:libyaml
fi

if [[ -n `command -v apt` ]]; then
    apt update
    apt install -y libmemcached libmemcached-devel memcached
else if [[ -n `command -v yum` ]]; then
    yum makecache
    yum install -y libmemcached libmemcached-devel memcached
else
    echo "ERROR: No package manager was found"
    need_manual=$need_manual:package_manager
fi

echo $need_manual
