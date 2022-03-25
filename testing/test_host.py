"""
test_host.py

This script allows for launching tests running either on both emulated and
physical guests running on QEMU or over UART (respectively).
"""
import argparse

def main():
    parser = argparse.ArgumentParser(description="Swink Test Host")
    parser.add_argument(
        "type", metavar="T",
        help="The type of the guest to run . For QEMU guests, enter 'qemu'. For " +
        "physical guests, enter the path to the UART serial device."
    )

if __name__ == "__main__":
    main()