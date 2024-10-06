#!/bin/bash

help() {
  cat << EOF
usage: $0 [OPTIONS] [EXTRA_ARGS]

    -h, --help           Show this message
    --cmd                west command: [build, flash, etc..]
    -b, --board          Board to build against.
    -p                   Clean build.

Examples:
    $0 --cmd=build --board=esp32_devkitc_wroom
    $0 --cmd=flash
    $0 --cmd=mon  (runs espressif monitor, if esp32)
    $0 --cmd=flashmon  (runs flash then espressif monitor, if esp32)
    $0 --cmd=<westcmd> [westcmd opts]
    $0 --cmd=manifest --path
    $0 --cmd=build -t menuconfig
EOF
}

init_script=init_venv.sh

# Search up through directory tree for init script.
function init_ws {
    found_init=0
    start_dir=$(pwd)
    current_dir=$(pwd)
    while [[ "$current_dir" != "/" ]]; do
        if [[ -f "$current_dir/$init_script" ]]; then
            echo "Found $init_script at $current_dir, sourcing."
            source $current_dir/$init_script
            found_init=1
            cd $start_dir
            break
        fi
        # Move 1 directory up.
        cd ..
        #current_dir=$(dirname "$current_dir")
        current_dir=$(pwd)
    done

    if [[ $found_init == 0 ]]; then
        echo "Could not find $init_script in directory tree".
        exit
    fi
}

# Activate the virtual env.
init_ws

board=
cleanbuild=
cmd=

leftoverargs=()
for arg in "$@"; do
    case $arg in
        --cmd=*)
            cmd="${arg#*=}"
            shift
            ;;
        -b=*|--board=*)
            board_name="${arg#*=}"
            board="-b $board_name"
            shift
            ;;
        -p)
            cleanbuild="-p"
            ;;
        -h|--help)
            help
            return 0
            ;;
        *)
            leftoverargs+=("$arg")
            shift
            ;;
    esac
done

if [[ $cmd == "mon" ]]; then
    cmd="espressif monitor"
    cmdline="west $cmd $cleanbuild $board "${leftoverargs[@]}""
    $cmdline
    exit 0
fi

if [[ $cmd == "flashmon" ]]; then
    cmd=flash
    cmdline="west $cmd $cleanbuild $board "${leftoverargs[@]}""
    $cmdline

    cmd="espressif monitor"
    cmdline="west $cmd $cleanbuild $board "${leftoverargs[@]}""
    $cmdline
    exit 0
fi

cmdline="west $cmd $cleanbuild $board "${leftoverargs[@]}""
echo $cmdline
$cmdline
