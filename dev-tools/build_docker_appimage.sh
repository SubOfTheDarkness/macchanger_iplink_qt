#!/bin/bash
set -e

if ! docker image inspect appimage-builder &> /dev/null; then
    echo "[*] Создание сборочного контейнера Ubuntu 22.04 для AppImage..."
    
    cat << 'EOF' > Dockerfile.appimage.tmp
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install -y cmake make g++ qt6-base-dev libx11-dev libxext-dev libxpm-dev wget file fuse libgl1-mesa-dev iproute2 procps && \
    rm -rf /var/lib/apt/lists/*

RUN wget -O /usr/bin/linuxdeployqt https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage && \
    chmod +x /usr/bin/linuxdeployqt

RUN wget -O /usr/bin/appimagetool https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage && \
    chmod +x /usr/bin/appimagetool
EOF

    DOCKER_BUILDKIT=1 docker build --progress=plain -t appimage-builder -f Dockerfile.appimage.tmp .
    rm -f Dockerfile.appimage.tmp
fi

echo "[*] Запуск автоматической сборки AppImage..."

docker run --rm \
    --device /dev/fuse \
    --cap-add SYS_ADMIN \
    -e APPIMAGE_EXTRACT_AND_RUN=1 \
    -v "$(pwd)":/workspace \
    appimage-builder /bin/bash -c "
        cd /workspace && \
        
        cmake -S . -B build-appimage -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/workspace/build-appimage/AppDir/usr && \
        cd build-appimage && \
        
        make -j\$(nproc) && \
        make install && \

        cp ./AppDir/usr/share/pixmaps/macchanger-toolkit.png ./AppDir/macchanger-toolkit.png && \

        export VERSION=\"1.0.1\"
        
        linuxdeployqt ./AppDir/usr/share/applications/macchanger.desktop \
            -unsupported-allow-new-glibc \
            -qmake=/usr/lib/qt6/bin/qmake

        appimagetool ./AppDir ./macchanger-toolkit-x86_64.AppImage
    "

echo "======================================================="
echo " Сборка AppImage успешно завершена"
echo " См. build-appimage/"
echo "======================================================="
