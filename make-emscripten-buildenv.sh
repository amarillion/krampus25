#!/bin/bash

IMAGE=amarillion/alleg5-emscripten:latest
SRCDIR=$(dirname $(readlink -f $0))
docker run \
	-u $(id -u):$(id -g) \
	-v $SRCDIR:/data \
	-v $SRCDIR/../twist5:/data/twist5 \
	$IMAGE \
	make TWIST_HOME=./twist5 TARGET=EMSCRIPTEN BUILD=DEBUG "$@"

	# -ti $IMAGE \
	# /bin/bash



