# juCi++ Installation Guide

- Installation
  - Linux
    - [Debian/Linux Mint/Ubuntu](#debianlinux-mintubuntu)
    - [Arch Linux/Manjaro Linux](#arch-linuxmanjaro-linux)
    - [Fedora](#fedora)
    - [Mageia](#mageia)
    - [OpenSUSE Tumbleweed](#opensuse-tumbleweed)
    - [GNU Guix/GuixSD](#gnu-guixguixsd)
  - [FreeBSD](#freebsd)
  - [NetBSD](#netbsd)
  - MacOS
    - [Homebrew](#macos-with-homebrew-httpbrewsh)
  - Windows
    - [MSYS2](#windows-with-msys2-httpsmsys2githubio)
- [Run](#run)

## Debian/Linux Mint/Ubuntu

Install dependencies:

```sh
sudo apt-get install libclang-dev liblldb-dev || sudo apt-get install libclang-6.0-dev liblldb-6.0-dev || sudo apt-get install libclang-4.0-dev liblldb-4.0-dev || sudo apt-get install libclang-3.8-dev liblldb-3.8-dev
sudo apt-get install universal-ctags || sudo apt-get install exuberant-ctags
sudo apt-get install git cmake make g++ clang-format pkg-config libboost-filesystem-dev libboost-serialization-dev libgtksourceviewmm-3.0-dev aspell-en libaspell-dev libgit2-dev
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake -DCMAKE_CXX_COMPILER=g++ ..
make
sudo make install
```

## Arch Linux/Manjaro Linux

Install dependencies:

```sh
sudo pacman -S git cmake pkg-config make clang lldb gtksourceviewmm boost aspell aspell-en libgit2 ctags
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake ..
make
sudo make install
```

## Fedora

Install dependencies:

```sh
sudo dnf install git cmake make gcc-c++ clang-devel clang lldb-devel boost-devel gtksourceviewmm3-devel gtkmm30-devel aspell-devel aspell-en libgit2-devel ctags
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake -DCMAKE_CXX_COMPILER=g++ ..
make
sudo make install
```

## Mageia

**Mageia might not yet support LLDB, but you can compile without debug support.**

Install dependencies:

32-bit:

```sh
sudo urpmi git cmake make gcc-c++ clang libclang-devel libboost-devel libgtkmm3.0-devel libgtksourceviewmm3.0-devel libaspell-devel aspell-en libgit2-devel
```

64-bit:

```sh
sudo urpmi git cmake make gcc-c++ clang lib64clang-devel lib64boost-devel lib64gtkmm3.0-devel lib64gtksourceviewmm3.0-devel lib64aspell-devel aspell-en libgit2-devel
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake -DCMAKE_CXX_COMPILER=g++ ..
make
sudo make install
```

## OpenSUSE Tumbleweed

Install dependencies:

```sh
sudo zypper install git-core cmake gcc-c++ boost-devel libboost_filesystem-devel libboost_serialization-devel clang-devel lldb-devel lldb gtksourceviewmm3_0-devel aspell-devel aspell-en libgit2-devel ctags
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake -DCMAKE_CXX_COMPILER=g++ ..
make
sudo make install
```

## GNU Guix/GuixSD

Simply install juCi++ from the official package definition

```sh
guix install jucipp
```

## FreeBSD

On FreeBSD, latest release of juCi++ is available through the port: jucipp.

## NetBSD

Install dependencies:

```sh
pkgin -y install lldb ccache doxygen pkg-config aspell-en libgit2 gtksourceviewmm exctags boost
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir -p jucipp/build
cd jucipp/build
cmake -DCMAKE_INSTALL_PREFIX=/usr/pkg ..
make
su -m
make install
```

## MacOS with Homebrew (http://brew.sh/)

Install dependencies:

```sh
brew install cmake pkg-config boost gtksourceviewmm3 gnome-icon-theme aspell llvm clang-format libgit2 zlib libxml2 universal-ctags
```

Mojave users might need to install headers:

```sh
open /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg
```

Get juCi++ source, compile and install:

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake ..
make
make install
```

## Windows with MSYS2 (https://msys2.github.io/)

**See https://gitlab.com/cppit/jucipp/issues/190 for details on adding debug support in MSYS2**

Install dependencies (replace `x86_64` with `i686` for 32-bit MSYS2 installs):

```sh
pacman -S git mingw-w64-x86_64-cmake make mingw-w64-x86_64-toolchain mingw-w64-x86_64-clang mingw-w64-x86_64-gtkmm3 mingw-w64-x86_64-gtksourceviewmm3 mingw-w64-x86_64-boost mingw-w64-x86_64-aspell mingw-w64-x86_64-aspell-en mingw-w64-x86_64-libgit2 mingw-w64-x86_64-universal-ctags-git
```

Note that juCi++ must be built and run in a MinGW Shell (for instance MinGW-w64 Win64 Shell).

Get juCi++ source, compile and install (replace `mingw64` with `mingw32` for 32-bit MSYS2 installs):

```sh
git clone --recursive https://gitlab.com/cppit/jucipp
mkdir jucipp/build
cd jucipp/build
cmake -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=/mingw64 ..
make
make install
```

## Run

```sh
juci
```

Alternatively, you can also include directories and files:

```sh
juci [directory] [file1 file2 ...]
```
