# MacChanger (Qt Tool)
[Читать на русском](README_RU.md)

A cross-platform (Linux-oriented) graphical C++/Qt6 application designed for fast MAC address modification, hardware address profile management, and multi-threaded network diagnostics.

## Features

* **Network Interface Manager:** Automatically detects active network interfaces in the system (excluding loopback `lo`).
* **MAC Profile Management:** Read and save custom aliases for MAC addresses via local INI configuration files.
* **Flexible Address Changing:** Supports manual MAC address input with strict mask validation, pre-configured profile switching, or quick reset to the factory/default hardware address.
* **Secure Execution:** System settings are applied via `pkexec`, requesting root privileges only at the exact moment of modification.
* **System Tray Integration:** Automatically checks for system tray availability. Minimizes to the tray on close, provides a context menu for deployment, hot restarting, or exiting.
* **Multi-threaded Diagnostics (Ping):** Supports dynamic sub-tabs (`№1`, `№2`...) to ping multiple targets simultaneously. Each tab is fully autonomous, auto-detects the active gateway (`default via`), and manages its own isolated ping process.
* **Keyboard-Driven Workflow:** Advanced hotkey support allows full control over the application without using a mouse.

## Hotkeys (Shortcuts)
* `Ctrl + Q` — Full application closure (unloads from memory).
* `Ctrl + W` — Close window (minimizes to system tray).
* `Ctrl + T` — Create a new network monitoring tab (active inside the Ping tab).
* `Ctrl + Shift + W` — Close the current active sub-tab.
* `Ctrl + R` — Re-read the configuration file from disk (active inside the MacChanger tab).
* `Ctrl + Shift + R` — Trigger a hot restart of the entire application.
* `Ctrl + Return` — Apply the MAC address or start/stop pinging (depending on the active tab).

------------------------------

## Project Architecture
The project is built on strict OOP principles with isolated modules:

* `CMakeLists.txt` — Main build configuration script.
* `main.cpp` — Application entry point, CLI flag handling, and main widget initialization.
* `macchanger_widget.h / .cpp / .ui` — Main window logic, profile management, tray, and shortcuts.
* `ping_tab.h / .cpp / .ui` — Autonomous, isolated ping tab widget with its own `QProcess`.
* `resources.qrc` — Qt resource file that compiles the fallback configuration into the binary.
* `default_config.ini` — Default fallback configuration file.

## Configuration (INI)
On the first run, the app creates a physical configuration file at `~/.config/macchanger/address_aliases.ini`. If empty, it imports settings from app resources:

* `[DEFAULTS]` — Stores the default interface and original factory MAC addresses for quick restoration.
* `[MAC_ALIASES]` — User-defined named profiles (e.g., "Home_Router", "Work_AP").

------------------------------

## Build and Run
### System Requirements

* A compiler supporting the **C++17** standard (GCC / Clang).
* **CMake** build system (version 3.16 or higher).
* **Qt 6** framework (Core, Gui, and Widgets components).
* Required system utilities: `iproute2` (the `ip` command), `iputils-ping` (`ping`), and `policykit-1` (`pkexec`).

### Build Instructions

1. Create a build directory and enter it:
   ```sh
   mkdir build && cd build
   ```
2. Generate build files via CMake:
   ```sh
   cmake ..
   ```
3. Compile the project:
   ```sh
   make
   ```
   
### CLI Flags
The application can be started directly in a minimized mode using the command-line flag:
```sh
./MacChanger --tray
```
*Note: If the system tray is unavailable in your desktop environment, the `--tray` flag will be ignored, and the application will start in normal windowed mode.*

### Package Generation

#### 1. Generating .deb Package (For Debian / LMDE / Ubuntu)
The project utilizes CMake's built-in **CPack** module to automatically package the application.
To build a native `.deb` package that handles all system dependencies and adds a menu shortcut:
```sh
mkdir -p build && cd build
cmake ..
make
make package
```
This will produce a `macchanger-toolkit-<version>-amd64.deb` file inside the `build` directory.

#### 2. Generating Arch Linux Package
A local `PKGBUILD` script is included in the repository root. To compile and clean-install the native Arch package using `pacman` directly from your local sources:
```sh
makepkg -si
```

---

## Security Notice
To modify the MAC address, the application executes system commands (`ip link set dev ...`). When clicking "Apply", the OS invokes a graphical polkit window (`pkexec`) to authorize the user with root privileges. Without entering the correct administrator password, the MAC address will not be changed.

If you wish to cache the password for `pkexec` (valid for 5 minutes) so you don't have to enter it every time, you can add a polkit rule (**at your own risk**):
```sh
echo -e 'polkit.addRule(function(action, subject) {\n    if (action.id == "org.freedesktop.policykit.exec") {\n        return polkit.Result.AUTH_ADMIN_KEEP;\n    }\n});' | sudo tee /etc/polkit-1/rules.d/50-pkexec-global.rules
```
