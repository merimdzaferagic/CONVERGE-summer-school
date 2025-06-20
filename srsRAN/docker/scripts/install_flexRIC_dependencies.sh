#!/bin/bash

set -e

main() {

    # Check number of args
    if [ $# != 0 ] && [ $# != 1 ]; then
        echo >&2 "Illegal number of parameters"
        echo >&2 "Run like this: \"./install_dependencies.sh [<mode>]\" where mode could be: build, run and extra"
        echo >&2 "If mode is not specified, all dependencies will be installed"
        exit 1
    fi

    local mode="${1:-all}"

    . /etc/os-release

    echo "== Installing flexRIC dependencies, mode $mode =="

    if [[ "$ID" == "debian" || "$ID" == "ubuntu" ]]; then
        if [[ "$mode" == "all" || "$mode" == "build" ]]; then
            DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y --no-install-recommends \
                swig libsctp-dev python3 build-essential cmake gcc cmake-curses-gui python3-dev pkg-config libconfig-dev libconfig++-dev 
        fi
    else
        echo "OS $ID not supported"
        exit 1
    fi

}

main "$@"
