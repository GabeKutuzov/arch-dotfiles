#!/usr/bin/bash

make -f maklux64 clean &&
make -f maklux64 &&
make -f maklux64 Rkgtest
