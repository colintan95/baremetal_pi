#!/usr/bin/env bash

path=/media/colintan/pi

sudo mount $path 
cp out/kernel8.img $path 
sudo umount $path
