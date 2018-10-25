#!/bin/bash
BUILD_ENV="1.0.0"
BUILD_GENERATOR="Ninja"
SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
mkdir build-env
cd build-env
wget https://github.com/prusa3d/MM-build-env/releases/download/$BUILD_ENV/MM-build-env-Linux64-$BUILD_ENV.zip
unzip MM-build-env-Linux64-$BUILD_ENV.zip -d ../../MM-build-env-$BUILD_ENV
cd ../../MM-build-env-$BUILD_ENV
BUILD_ENV_PATH="$( pwd -P )"
export PATH=$BUILD_ENV_PATH/cmake/bin:$BUILD_ENV_PATH/ninja:$BUILD_ENV_PATH/avr/bin:$PATH
cd ..
mkdir MM-control-01-build
cd MM-control-01-build
touch $BUILD_ENV.version
cmake -G ""$BUILD_GENERATOR"" $SCRIPT_PATH
cmake --build .