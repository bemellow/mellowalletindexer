#!/bin/sh
g++ Blockchain.cpp cpphelper.cpp SHA256.cpp TimestampIndex.cpp -o libcpphelper.so -O3 -shared -fPIC
