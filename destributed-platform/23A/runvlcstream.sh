#!/bin/bash -

cd ~/Desktop/PlaneVideos/ && cvlc -vvv 1low.mp4  --sout '#transcode{vcodec=h265,scale=Auto,acodec=none}:udp{dst=10.101.0.13:1234}' --sout-keep --loop &

