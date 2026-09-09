# MacChanger ToolKit

A cross-platform graphical C++/Qt6 application for fast MAC address modification, hardware profile management, and multi-threaded network diagnostics.

* * *

## Features, Shortcuts and User Guide

The application features automated interface detection, profile management via INI files, secure root execution via pkexec, multi-threaded diagnostics, and system tray integration with comprehensive keyboard shortcuts and operational guides for MAC spoofing, network scanning, and node monitoring.

* * *

## Deep Dive & Kernel Engineering

<details>
<summary><b>1. MAC Spoofer & Interface State Cycle</b></summary>

The app changes the MAC address by executing system commands through `pkexec` to get root privileges.

### How it works
- **The Cycle:** Linux won't let you change the MAC of an active card. The app safely brings the interface **down**, writes the new 48-bit address to the kernel config, and brings the interface back **up** (forcing a fresh DHCP request to get a new IP).
- **Smart Randomization:** When generating a random MAC, the first byte is strictly limited to specific values (`0x02`, `0x06`, `0x0A`, `0x0E`). This forces the address flags to register as *Locally Administered* and *Unicast*, preventing routers and managed switches from dropping your traffic.

</details>

**TL;DR:** Changing MAC requires a quick down-set-up cycle. Random addresses are automatically masked so network routers accept them as normal valid devices.

<details>
<summary><b>2. Network Scanner & ARP Cache Harvesting</b></summary>

The scanner maps your local network instantly without using aggressive port-scanning tools that trigger firewalls.

### How it works
- **The Subnet Wave:** The app reads your current network mask via `QNetworkInterface` to find your local `/24` subnet. It then fires rapid, parallel `ping -c 1 -W 1` commands to all 254 possible IP addresses at once.
- **Kernel Reading:** The app doesn't even care about the ping response logs. The goal is just to force local devices to reply. When they do, the Linux kernel automatically adds them to the system ARP table. The app then simply reads the ready list directly from `/proc/net/arp` without needing root.

</details>

**TL;DR:** Instead of heavy port scanning, the app pings the whole subnet in 1 second to force the OS to build an ARP table, then parses `/proc/net/arp` directly.

<details>
<summary><b>3. Latency Monitor & UI Protection</b></summary>

The ping engine tracks connection drops, checks line stability, and updates the tray.

### How it works
- **UI Protection:** The application monitors latency (RTT) and automatically formats the output. If your connection delays shoot past 9999 ms (or drop completely), the text display automatically switches from milliseconds to clean floating-point seconds (e.g., `14.25 s`) to keep the interface from breaking.
- **Centralized Alerts:** An isolated `QTimer` catches connection drops. If a monitored host goes silent, it flags the UI widgets as dangerous (changing CSS colors to red) and fires a clean, batched warning to the system tray without spamming notifications.

</details>

**TL;DR:** The pinger tracks line stability, switches UI layout to seconds if lag is massive, and uses a single timer to safely batch tray notifications without spamming your desktop environment.

* * *

## Build, Packaging & Automation Tools

### System Requirements

- **Compiler:** GCC / Clang with **C++17** standard support.
- **Build System:** CMake (version 3.16 or higher).
- **Framework:** Qt 6 (Core, Gui, Widgets, Network).
- **System Utilities:** `iproute2` (`ip` command), `iputils-ping` ( `ping`), and `policykit-1` ( `pkexec`).

### Standard Native Build

To compile the binary locally on your host machine:

```sh
mkdir build && cd build
cmake ..
make
./macchanger-toolkit
```

#### Native Arch Linux Packaging

The repository includes a root `PKGBUILD` script.  
Run inside the repository root to compile and clean-install:
```sh
makepkg -si
```

## Dev-Tools

All automation scripts are containerized using Docker to eliminate host dependency issues.

<details>
<summary><b>Automated DEB Packaging</b></summary>

Uses a reusable Docker image (`macchanger-builder`) based on Ubuntu 24.04 to compile the project and generate a native `.deb` package via CPack.

```sh
# Run from repository root to build the DEB package:
./dev-tools/build_deb.sh
```
</details>

<details>
<summary><b>DEB Testing</b></summary>

Uses a reusable Docker image (`macchanger-tester`) based on Ubuntu 24.04 to spin up a clean environment. It temporarily shares your host screen socket (`xhost +local:docker`), installs the compiled `.deb` package, and launches the app to verify its UI behavior.

```sh
# Run from repository root to verify the DEB installation:
./dev-tools/test_deb.sh
```
</details>

<details>
<summary><b>Docker AppImage Construction</b></summary>

Uses a reusable Docker image (`appimage-builder`) based on Ubuntu 22.04 for maximum GLIBC backward compatibility. It bundles dependencies into a portable AppImage using `linuxdeployqt` and `appimagetool`.

```sh
# Run from repository root to build the AppImage:
./dev-tools/build_docker_appimage.sh
```
</details>