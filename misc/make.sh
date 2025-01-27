#!/bin/bash

NO=$(apt-cache policy binutils-dev | grep Installed | grep -v none)
if [ -n $NO ]; then
  sudo apt install binutils-dev
fi

make -f example.mk all
make -f objtool.mk all

make -f example.mk DEBUG=y all
make -f objtool.mk DEBUG=y all

