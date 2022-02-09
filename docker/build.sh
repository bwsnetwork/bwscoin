#!/bin/bash

if [ -z "$GITHUB_ACCESS_TOKEN" ]
then
  echo "Please define GITHUB_ACCESS_TOKEN to download source archive from private repository."
  exit
fi

if [ "$#" -ne 2 ]
then
    echo "Usage: build.sh <version> <branch>"
    exit -1
fi
CWD=$(dirname "$(readlink -f $0)") && cd $CWD && \
echo $CWD && \
docker build --network=host --rm -t bwscoin-build -f Dockerfile.build . && \
rm -f *.deb && \
docker run --network=host -e GITHUB_ACCESS_TOKEN=$GITHUB_ACCESS_TOKEN -e VERSION=$1 -e BRANCH=$2 --rm --volume $CWD:/bwscoin -i -t bwscoin-build && \
docker build --network=host --rm -t $2:$1 -f Dockerfile .
