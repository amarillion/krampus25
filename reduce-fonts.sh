#!/bin/bash

# Prep: apt install python3-fonttools
#

for i in origdata/DejaVuSans*.ttf
do
BASE=`basename $i`
# Source: https://stackoverflow.com/a/48492035
pyftsubset origdata/$BASE \
      --unicodes=U+0000-024F,U+2000-20CF \
      --output-file=data/$BASE
done
