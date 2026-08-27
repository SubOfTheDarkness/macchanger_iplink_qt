# Maintainer: SubOfTheDarkness <204970490+SubOfTheDarkness@users.noreply.github.com>
pkgname=macchanger-toolkit
pkgver=1.0.2
pkgrel=1
pkgdesc="Fast MAC address changer and network ping toolkit (Qt6/CMake)"
arch=('x86_64')
url="https://github.com/SubOfTheDarkness/macchanger_iplink_qt"
license=('GPL-3.0-or-later')
depends=('qt6-base' 'iproute2' 'iputils' 'polkit')
makedepends=('cmake')
options=('!debug')

source=()
sha256sums=()

build() {
  cmake -B build -S "$startdir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
