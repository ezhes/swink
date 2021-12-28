#!/usr/bin/env bash

DEBUG_ARG=''
if [[ "$1" == "-d" ]]; then
	DEBUG_ARG='-S -s'
fi

echo "Starting QEMU (press ctr-A, X to terminate)..."
qemu-system-aarch64 -M raspi3 -kernel build/kernel.img -nographic -serial null -serial mon:stdio $DEBUG_ARG
echo "QEMU exited"
