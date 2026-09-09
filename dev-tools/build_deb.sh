#!/bin/bash
set -e

if ! docker image inspect macchanger-builder &> /dev/null; then
    echo "[*] Creating local Docker build image for MacChanger..."
    
    cat << 'EOF' > Dockerfile.tmp
FROM ubuntu:24.04
RUN apt-get update -y && \
    apt-get install -y cmake make g++ qt6-base-dev libx11-dev && \
    rm -rf /var/lib/apt/lists/*
EOF

    DOCKER_BUILDKIT=1 docker build -t macchanger-builder -f Dockerfile.tmp .
    rm -f Dockerfile.tmp
    echo "[+] Build image successfully created"
fi

echo "[*] Launching MacChanger compilation pipeline..."

docker run --rm \
    -v "$(pwd)":/workspace \
    macchanger-builder /bin/bash -c "
        cd /workspace && \
        cmake -S . -B build-deb -DCMAKE_BUILD_TYPE=Release && \
        cd build-deb && \
        make package
    "

echo "======================================================="
echo "[+] MacChanger DEB build completed successfully!"
echo "$(ls build-deb/*.deb 2>/dev/null || echo '[-] Error: Package not found')"
echo "======================================================="