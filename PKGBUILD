# Maintainer: SubOfTheDarkness <204970490+SubOfTheDarkness@users.noreply.github.com>
pkgname=macchanger-toolkit
pkgver=1.0.4
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
  cmake -B "$startdir/build-arch" \
        -S "$startdir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
        
  cmake --build "$startdir/build-arch"
}

package() {
  DESTDIR="$pkgdir" cmake --install "$startdir/build-arch"
}
