#!/bin/bash

set -e

PLUGIN_ID="org.kde.macchanger.qml"
PLASMOID_NAME="${PLUGIN_ID}.plasmoid"
WIDGET_DIR="org.kde.macchanger.qml"
PROJECT_ROOT=$(pwd)

DO_PACK=false
DO_INSTALL=false
DO_CLEAN=false
DO_UNINSTALL=false
DO_TEST=false
DO_PURGE=false

show_help() {
    echo "Usage: ./pack.sh [options]"
    echo "Options:"
    echo "  -p        Pack the project into a .plasmoid ZIP archive"
    echo "  -i        Install/reinstall the packed .plasmoid into Plasma 6"
    echo "  -c        Clean packed archive"
    echo "  -r        Uninstall plasmoid"
    echo "  -t        Start testing in plasmoidviewer"
    echo "  -e        Purge Plasma QML Cache"
    echo "  -h        Show this help message"
}

if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -*)
            arg="$1"
            for (( i=1; i<${#arg}; i++ )); do
                char="${arg:i:1}"
                case "$char" in
                    p) DO_PACK=true ;;
                    i) DO_INSTALL=true ;;
                    h) show_help; exit 0 ;;
                    c) DO_CLEAN=true ;;
                    r) DO_UNINSTALL=true ;;
                    t) DO_TEST=true ;;
                    e) DO_PURGE=true ;;
                    *) echo "Error: Unknown option -$char"; show_help; exit 1 ;;
                esac
            done
            ;;
        *)
            echo "Error: Invalid argument $1"
            show_help
            exit 1
            ;;
    esac
    shift
done



if [ "$DO_UNINSTALL" = true ]; then
    echo "=== Uninstalling plasmoid ==="
    if kpackagetool6 --type Plasma/Applet --list 2>/dev/null | grep -q "${PLUGIN_ID}"; then
        kpackagetool6 --type Plasma/Applet --remove "${PLUGIN_ID}" 2>/dev/null || true
        echo "Plasmoid ${PLUGIN_ID} successfully uninstalled."
    else
        echo "Plasmoid ${PLUGIN_ID} is not installed."
    fi
fi



if [ "$DO_PURGE" = true ]; then
    echo "=== PURGING PLASMA QML CACHE ==="
    rm -rf "$HOME/.cache/qmlcache"
    rm -rf "$HOME/.cache/plasma*"
    
    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.refreshCurrentShell 2>/dev/null || true
fi



if [ "$DO_CLEAN" = true ]; then
    echo "=== Cleaning packed archive ==="
    if [ -f "${PLASMOID_NAME}" ]; then
        rm -f "${PLASMOID_NAME}"
        echo "Removed ${PLASMOID_NAME}"
    else
        echo "Nothing to clean."
    fi
fi



if [ "$DO_PACK" = true ]; then
    echo "=== Checking dependencies ==="
    if ! command -v zip &> /dev/null; then
        echo "=========================================================="
        echo "ERROR: 'zip' utility is not found in your system!"
        echo "Please install it manually using your package manager:"
        echo "  Arch:   sudo pacman -S zip"
        echo "  Fedora: sudo dnf install zip"
        echo "  Ubuntu: sudo apt install zip"
        echo "=========================================================="
        exit 1
    fi
fi



if [ "$DO_PACK" = true ]; then
    echo "=== Packing project ==="
    rm -f "${PLASMOID_NAME}"

    if [ ! -d "${WIDGET_DIR}" ]; then
        echo "Error: Directory ${WIDGET_DIR} not found!"
        exit 1
    fi

    cd "${WIDGET_DIR}"
    zip -q -r "${PROJECT_ROOT}/${PLASMOID_NAME}" metadata.json contents
    cd "${PROJECT_ROOT}"

    echo "Successfully packed: ${PLASMOID_NAME}"
fi



if [ "$DO_INSTALL" = true ]; then
    echo "=== Installing to Plasma 6 ==="
    
    if [ ! -f "${PLASMOID_NAME}" ]; then
        echo "Error: ${PLASMOID_NAME} not found! Pack it first using -p option."
        exit 1
    fi

    echo "Registering widget custom icon..."
    mkdir -p "$HOME/.local/share/pixmaps"
    if [ -f "${WIDGET_DIR}/icon.png" ]; then
        cp -f "${WIDGET_DIR}/icon.png" "$HOME/.local/share/pixmaps/macchanger-widget-icon.png"
    elif [ -f "icon.png" ]; then
        cp -f icon.png "$HOME/.local/share/pixmaps/macchanger-widget-icon.png"
    else
        echo "Warning: icon.png not found, skipping icon registration."
    fi

    echo "Installing widget to local profile..."
    if kpackagetool6 --type Plasma/Applet --list 2>/dev/null | grep -q "${PLUGIN_ID}"; then
        kpackagetool6 --type Plasma/Applet --remove "${PLUGIN_ID}" 2>/dev/null || true
    fi

    kpackagetool6 --type Plasma/Applet --install "${PLASMOID_NAME}"
    echo "=== Widget is successfully installed ==="
fi



if [ "$DO_TEST" = true ]; then
    echo "=== Starting testing sandbox in desktop mode ==="
    plasmoidviewer -a "${PLUGIN_ID}"
fi
