# Maintainer: SubOfTheDarkness <204970490+SubOfTheDarkness@users.noreply.github.com>
pkgname=macchanger-toolkit
pkgver=1.0.0
pkgrel=1
pkgdesc="Fast MAC address changer and network ping toolkit (Qt6/CMake)"
arch=('x86_64')
url="https://github.com/SubOfTheDarkness/macchanger_iplink_qt"
license=('GPL')
depends=('qt6-base' 'iproute2' 'iputils' 'polkit')
makedepends=('cmake')

source=("local_sources::dir://.")
sha256sums=('SKIP')

build() {
  cmake -B build -S "$srcdir/local_sources" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
