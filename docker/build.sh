#!/bin/bash

if [ "$#" -ne 2 ]
then
    echo "Usage: build.sh <version> <branch>"
    exit -1
fi
CWD=$(dirname "$(readlink -f $0)") && cd $CWD && \
echo $CWD && \
docker build --network=host --rm -t bwscoin-build -f Dockerfile.build . && \
rm -f *.deb && \
docker run --network=host -e VERSION=$1 -e BRANCH=$2 --rm --volume $CWD:/bwscoin -i -t bwscoin-build && \
docker build --network=host --rm -t $2:$1 -f Dockerfile .
