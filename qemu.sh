#!/usr/bin/env bash

DEBUG_ARG=''
if [[ "$1" == "-d" ]]; then
	DEBUG_ARG='-S -s'
fi

echo "Starting QEMU (press ctr-A, X to terminate)..."
qemu-system-aarch64 \
	-M raspi3b \
	-kernel build/kernel/kernel8.img \
	-nographic \
	-serial null \
	-serial mon:stdio \
	-semihosting $DEBUG_ARG

QEMU_EXIT=$?
echo "QEMU exited (status = $QEMU_EXIT)"
exit $QEMU_EXIT
