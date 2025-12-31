import sys
import os
import shutil
from invoke import task


def git_clone_if_missing(c, url, directory):
    if not os.path.exists(directory):
        print(f"Cloning {directory}...")
        c.run(f"git clone {url} {directory}")


def run_in_docker(
    c, cmd, image="riscv-ecu-renode", interactive=False, privileged=False, pty=False
):
    cwd = os.getcwd()
    flags = "--rm"
    if interactive:
        flags += " -i"
    if privileged:
        flags += " --privileged"

    docker_cmd = (
        f"docker run {flags} " f'-v "{cwd}:/app" ' f"{image} " f'bash -c "{cmd}"'
    )
    print(f"Running in Docker: {cmd}")
    c.run(docker_cmd, pty=pty)


@task
def fetch_aes_libraries(c):
    """Fetch tiny-AES-CMAC-c and tiny-AES-c libraries."""
    git_clone_if_missing(
        c, "https://github.com/elektronika-ba/tiny-AES-CMAC-c.git", "tiny-AES-CMAC-c"
    )
    git_clone_if_missing(c, "https://github.com/kokke/tiny-AES-c.git", "tiny-AES-c")


@task
def build_renode_base(c):
    """Clone renode-docker and build the base image."""
    git_clone_if_missing(
        c, "https://github.com/renode/renode-docker.git", "renode-docker"
    )

    print("Building renode:1.16 base image...")
    with c.cd("renode-docker"):
        c.run("docker build -t renode:1.16 .")


@task(pre=[build_renode_base])
def build_image(c):
    """Build the Docker image."""
    print("Building Docker image...")
    c.run("docker build -t riscv-ecu-renode .")


@task(pre=[fetch_aes_libraries])
def build_firmware(c):
    """Build the firmware explicitly."""
    cc = "riscv-none-elf-gcc"
    cflags = "-march=rv32i -mabi=ilp32 -nostdlib -g -O0 -Itiny-AES-CMAC-c -Itiny-AES-c"
    ldflags = "-T linker.ld -Wl,-melf32lriscv"
    sources = "start.s main.c tiny-AES-c/aes.c tiny-AES-CMAC-c/aes_cmac.c"
    output = "firmware.elf"

    cmd = f"{cc} {cflags} {ldflags} -o {output} {sources}"

    try:
        run_in_docker(c, cmd)
    except Exception as e:
        print("\nERROR: Building firmware.elf failed.")


@task(pre=[build_image, build_firmware])
def run(c):
    """Run the firmware in Renode inside Docker."""

    # Try to load vcan module (might fail if not privileged or missing in kernel).
    inner_cmd = (
        "set -e; "
        "echo 'Setting up vcan...'; "
        "if ! ip link show vcan0 >/dev/null 2>&1; then "
        "  if ! sudo ip link add dev vcan0 type vcan >/dev/null 2>&1; then "
        "    echo 'vcan0 creation failed. Attempting to load vcan module...'; "
        "    modprobe vcan >/dev/null 2>&1 || true; "
        "    sudo ip link add dev vcan0 type vcan; "
        "  fi; "
        "fi; "
        "sudo ip link set up vcan0; "
        "ip link show vcan0; "
        "echo 'Starting candump (background)...'; "
        "rm -f candump.log; "
        "candump -aed any,0:0,#FFFFFFFF | tee candump.log & "
        "echo 'Starting Renode...'; "
        "set +e; "
        "renode --disable-xwt --console run.resc; "
        "RET=$?; "
        "echo 'Renode exited. Stopping background tasks...'; "
        "echo 'Dumping candump log:'; "
        "cat candump.log; "
        "exit $RET"
    )

    use_pty = sys.platform != "win32"

    try:
        run_in_docker(c, inner_cmd, interactive=True, privileged=True, pty=use_pty)
    except Exception as e:
        print("\nERROR: Docker run failed.")


@task
def clean(c):
    if os.path.exists("firmware.elf"):
        os.remove("firmware.elf")
    if os.path.exists("tiny-AES-CMAC-c"):
        shutil.rmtree("tiny-AES-CMAC-c")
    if os.path.exists("tiny-AES-c"):
        shutil.rmtree("tiny-AES-c")
