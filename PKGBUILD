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

source=()
sha256sums=()
options=('!debug')

build() {
  find "$startdir" -maxdepth 1 ! -name "src" ! -name "pkg" ! -name "$pkgname*" -exec cp -t "$srcdir" -r {} + 2>/dev/null || true

  cmake -B build -S "$srcdir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}


package() {
  DESTDIR="$pkgdir" cmake --install build
}
