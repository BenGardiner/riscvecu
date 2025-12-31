# RISC-V ECU Firmware with Renode and Docker

This project demonstrates a simple RISC-V firmware that sends CAN messages. It is designed to be built and executed inside a Docker container using Renode for simulation. The Renode platform is 'fake' -- it has a UART and CAN controller which don't exist in a known RISCV SoC.

## Prerequisites

*   Linux or a custom WSL2 kernel with vcan support for running in Docker Desktop on Windows
*   Docker Desktop (Windows) or Docker Engine (Linux/Mac)
*   Python 3

## How to Run

1.  Install dependencies:
    ```bash
    pip install -r requirements.txt
    ```

2.  Run the simulation:
    ```bash
    invoke run
    ```

The `invoke run` command will:
1.  Build the Docker image `riscv-ecu-renode` (if needed).
2.  Start a container with `NET_ADMIN` privileges.
3.  Mount the current directory into the container.
4.  Create a `vcan0` interface inside the container.
5.  Build the firmware using `make`.
6.  Start `candump vcan0` in the background to monitor traffic.
7.  Run Renode with the `run.resc` script.

You should see Renode starting and `candump` outputting CAN messages if everything is configured correctly.
