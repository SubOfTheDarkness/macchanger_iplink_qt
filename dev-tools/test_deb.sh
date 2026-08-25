#!/bin/bash
set -e

if ! docker image inspect macchanger-tester &> /dev/null; then
    echo "Создание чистого рантайм-образа для тестов..."
    
    cat << 'EOF' > Dockerfile.tmp
FROM ubuntu:24.04
RUN apt-get update -y && rm -rf /var/lib/apt/lists/*
EOF

    DOCKER_BUILDKIT=0 docker build -t macchanger-tester -f Dockerfile.tmp .
    rm -f Dockerfile.tmp
    echo "Базовый тестовый образ успешно создан"
fi

xhost +local:docker > /dev/null

DEB_FILE=$(ls build-deb/*.deb 2>/dev/null | head -n 1)

if [ -z "$DEB_FILE" ]; then
    echo "Ошибка: .deb пакет не найден в папке build-deb/."
    exit 1
fi

echo "Найден пакет для тестирования: $DEB_FILE"

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
        echo 'Пакет macchanger-toolkit успешно установлен со всеми зависимостями' && \
        macchanger-toolkit
    "

xhost -local:docker > /dev/null