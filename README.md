# MacChanger QML (Plasma 6 Widget)

A fast, lightweight, and native KDE Plasma 6 widget designed for rapid MAC address modification and hardware address profile management directly from your system tray or panel. 

It acts as a standalone, zero-dependency frontend companion to the desktop application, using the exact same local configuration structure.

## Features

* **Plasma 6 Integration:** Built on clean QML, Qt Quick, and Kirigami components, matching your native system design and theme.
* **Network Interface Manager:** Automatically detects system network interfaces and displays the active MAC address in real-time.
* **Profile Sync:** Reads and applies named MAC address profiles directly from `~/.config/macchanger/address_aliases.ini` (compatible with the C++ application).
* **Reset to Native MAC:** Instantly reads the hardware defaults, dynamically injects fallback profiles, and re-reads settings on the fly.
* **Interface Info Dialog:** A built-in terminal-styled modal (`ip addr show`) that safely renders full diagnostic data without locking the UI.
* **Active Status Indicator:** Disables interaction buttons and activates a native `BusyIndicator` spinner while the Polkit auth process is in progress.
* **Clean MVC Architecture:** The logic layer is fully separated into isolated, stateless, and declarative modules for high performance and easy reading.

---

## Project Architecture
The QML-branch repository is decoupled into clean, modular components:

* `packer.sh` — Universal automation script to pack, clean, uninstall, or reinstall the widget.
* `metadata.json` — Plasma package manifest with project metadata, dependencies, and API requirements.
* `icon.png` — Asset source for the custom widget tray icon.
* `contents/config/default_config.ini` — Default local fallback configuration compiled inside the widget.
* `contents/ui/main.qml` — The root view orchestrating layout alignments, models, and reactive bindings.
* `contents/ui/BashExecutor.qml` — The controller layer encapsulating system pipes and standard buffers.
* `contents/ui/ConfigParser.js` — A stateless, pure JS INI parser.
* `contents/ui/MacEntryField.qml` — Custom masked text field with dynamic border error styling.
* `contents/ui/InterfaceInfoDialog.qml` — Terminal-styled overlay window with adaptive layouts.

---

## Automation Script (`packer.sh`)

### Usage and Examples

* **Pack only:** Generates a universal `.plasmoid` archive (standard ZIP format with exact root folder indexing required by Plasma 6).
  ```sh
  ./packer.sh -p
  ```
* **Install/Reinstall only:** Copies custom assets to system paths and registers the widget in your local Plasma cache.
  ```sh
  ./packer.sh -i
  ```
* **Clean archive:** Safely deletes local `.plasmoid` files.
  ```sh
  ./packer.sh -c
  ```
* **Uninstall from Plasma:** Purges the component completely from the desktop shell.
  ```sh
  ./packer.sh -r
  ```
* **Combined Chain Execution (Recommended):** You can stack options together in any order. The script will dynamically sort the logic to clean old artifacts, validate code, compile an archive, and trigger a hot reinstall at once:
  ```sh
  ./packer.sh -rcpi
  ```

---

## Installation & Deployment

### Local Shell Deployment (Any Distribution)
To deploy the extension locally on Arch Linux, Fedora, openSUSE, Ubuntu, or Debian, run the package sequence:
```sh
./packer.sh -pi
```
Once deployed, you can instantly spin up a local debugging sandbox environment to preview the panel without locking your layout:
```sh
plasmoidviewer -a org.kde.macchanger.qml
```
---

## Security Notice & Password Caching
To modify the MAC address, the widget executes system commands (`ip link set dev ...`). When clicking **"Apply Changes"**, the OS invokes a graphical polkit window (`pkexec`) to authorize the user with root privileges. Without entering the correct administrator password, the MAC address will not be changed.

If you wish to cache the password for `pkexec` (valid for 5 minutes) so you don't have to enter it every time you switch profiles via the widget, you can add a polkit rule (**at your own risk**):

```sh
echo -e 'polkit.addRule(function(action, subject) {\n    if (action.id == "org.freedesktop.policykit.exec") {\n        return polkit.Result.AUTH_ADMIN_KEEP;\n    }\n});' | sudo tee /etc/polkit-1/rules.d/50-pkexec-global.rules
```

---

### Core Prerequisites
Ensure your Linux environment has the following active system utilities:
* `iproute2` (delivers the `ip` binary)
* `policykit-1` (delivers the `pkexec` binary)
* `zip` (required strictly to compile the package archive via `packer.sh`)
