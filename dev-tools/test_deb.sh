#!/bin/bash
set -e

if ! docker image inspect macchanger-tester &> /dev/null; then
    echo "[*] Creating clean runtime Docker image for testing..."
    
    cat << 'EOF' > Dockerfile.tmp
FROM ubuntu:24.04
RUN apt-get update -y && rm -rf /var/lib/apt/lists/*
EOF

    DOCKER_BUILDKIT=0 docker build -t macchanger-tester -f Dockerfile.tmp .
    rm -f Dockerfile.tmp
    echo "[+] Base test image successfully created"
fi

xhost +local:docker > /dev/null

DEB_FILE=$(ls build-deb/*.deb 2>/dev/null | head -n 1)

if [ -z "$DEB_FILE" ]; then
    echo "[-] Error: .deb package not found in build-deb/ directory."
    exit 1
fi

echo "[*] Found target package for validation: $DEB_FILE"

docker run -it --rm \
    --net=host \
    -e DISPLAY=$DISPLAY \
    -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
    -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \
    -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
    -v $XDG_RUNTIME_DIR/$WAYLAND_DISPLAY:$XDG_RUNTIME_DIR/$WAYLAND_DISPLAY:ro \
    -v "$(pwd)":/workspace \
    macchanger-tester /bin/bash -c "
        apt-get update -y && \
        apt-get install -y /workspace/$DEB_FILE && \
        echo '[+] Package macchanger-toolkit installed successfully with all dependencies' && \
        macchanger-toolkit
    "

xhost -local:docker > /dev/null