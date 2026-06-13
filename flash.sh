#!/usr/bin/env bash

path=/pi

sudo mount $path
cp out/kernel8.img $path
sudo umount $path
