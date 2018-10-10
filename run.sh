#!/bin/sh
clang -sectcreate __TEXT __info_plist Info.plist -o server server.c
sudo codesign -s instrument ./server
sudo ./server /Users/domin568/Desktop/experiments/Instrumentation/test.obj 0x1C8 0x6F1