#!/bin/bash

set -e

for i in electrical network exclamation ; do
  convert $i.jpg -depth 1 -resize 48x48 -type bilevel BMP3:$i.bmp
  bin2c -C $i.c $i.bmp
done
