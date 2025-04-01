# This is an example PKGBUILD file. Use this as a start to creating your own,
# and remove these comments. For more information, see 'man PKGBUILD'.
# NOTE: Please fill out the license field for your package! If it is unknown,
# then please put 'unknown'.

# Maintainer: Your Name <youremail@domain.com>
pkgname=Wall-Haven-Desktop
pkgver=1.0
pkgrel=1
pkgdesc="UI to display wallhaven.cc api requests"
arch=('x86_64')
url="https://github.com/HaydenRoss/$pkgname"
license=('GPL')
depends=('qt6-base')
makedepends=('git' 'cmake' 'qt6-base')
install=
source=(https://github.com/HaydenRoss/$pkgname/archive/refs/tags/$pkgver-release.tar.gz)
md5sums=('bd9f785349bc0661605c053320ab3aba')

build() {
    cd "$srcdir/$pkgname-$pkgver-release"
    cmake .
	make
}

package() {
	cd "$srcdir/$pkgname-$pkgver-release"
	make DESTDIR="$pkgdir/" install
}