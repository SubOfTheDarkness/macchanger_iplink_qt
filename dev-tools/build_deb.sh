#!/bin/bash
set -e

if ! docker image inspect macchanger-builder &> /dev/null; then
    echo "Создание локального сборочного образа для MacChanger..."
    
    cat << 'EOF' > Dockerfile.tmp
FROM ubuntu:24.04
RUN apt-get update -y && \
    apt-get install -y cmake make g++ qt6-base-dev libx11-dev && \
    rm -rf /var/lib/apt/lists/*
EOF

    DOCKER_BUILDKIT=1 docker build -t macchanger-builder -f Dockerfile.tmp .
    
    rm -f Dockerfile.tmp
    echo "Образ для сборки успешно создан и сохранен"
fi

echo "Запуск компиляции проекта MacChanger..."

docker run --rm \
    -v "$(pwd)":/workspace \
    macchanger-builder /bin/bash -c "
        cd /workspace && \
        cmake -S . -B build-deb -DCMAKE_BUILD_TYPE=Release && \
        cd build-deb && \
        make package
    "

echo "======================================================="
echo "Сборка MacChanger успешно завершена"
echo "$(ls build-deb/*.deb 2>/dev/null || echo 'Пакет не найден')"
echo "======================================================="
