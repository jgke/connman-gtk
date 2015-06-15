#!/bin/sh

mkdir -p config

aclocal && \
    automake --add-missing --copy && \
    autoconf && \
    intltoolize --force

if [ -f config.status ]; then
    make clean
fi
