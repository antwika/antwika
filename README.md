# Readme

## Quick start

### Optional: Build the dev containers locally with **Docker**

```sh
$ docker build --no-cache -t antwika-dev-base:latest -f .devcontainer/base/Dockerfile .
```

Adjust gnu/llvm/mingw dockerfiles to use the locally built antwika-dev-base image

```sh
$ sed -i 's|FROM ghcr.io/antwika/antwika-dev-base:${BASE_VERSION}|FROM antwika-dev-base:${BASE_VERSION}|' \
  .devcontainer/gnu/Dockerfile \
  .devcontainer/llvm/Dockerfile \
  .devcontainer/mingw/Dockerfile
```

### Build the project

Open the project with **Visual Studio Code** and then reopen with in container:

```
Ctrl+Shift+P > Dev Containers: Reopen in Container
```

> You may need to delete the "apps/app/build" directory when switching between dev containers...

Build the project with

```
Ctrl+Shift+B
```

Now run the compiled binary `apps/app/build/Release/app` or `apps/app/build/Release/app.exe` on your target machine.

## Manual build steps

### Build Dev Containers

```sh
$ sed -i 's|FROM ghcr.io/antwika/antwika-dev-base:${BASE_VERSION}|FROM antwika-dev-base:${BASE_VERSION}|' \
  .devcontainer/gnu/Dockerfile \
  .devcontainer/llvm/Dockerfile \
  .devcontainer/mingw/Dockerfile
$ docker build --no-cache -t antwika-dev-base:latest -f .devcontainer/base/Dockerfile .
$ docker build --no-cache -t antwika-dev-gnu:latest -f .devcontainer/gnu/Dockerfile .
$ docker build --no-cache -t antwika-dev-llvm:latest -f .devcontainer/llvm/Dockerfile .
$ docker build --no-cache -t antwika-dev-mingw:latest -f .devcontainer/mingw/Dockerfile .
```

### Build project with GNU Dev Container

```sh
$ cd libs/log/
$ conan create . -pr:b=../../profiles/build/gcc-linux-x86_64 -pr:h=../../profiles/host/gcc-linux-x86_64 --build=missing
$ cd libs/engine/
$ conan create . -pr:b=../../profiles/build/gcc-linux-x86_64 -pr:h=../../profiles/host/gcc-linux-x86_64 --build=missing
$ cd ../../apps/app
$ conan install . -pr:b=../../profiles/build/gcc-linux-x86_64 -pr:h=../../profiles/host/gcc-linux-x86_64 --build=missing -s build_type=Release
$ cmake --preset conan-release
$ cmake --build build/Release
$ cd build/Release/
Run the build/Release/app on your Linux machine
```

### Build project with LLVM Dev Container

```sh
$ cd libs/log/
$ conan create . -pr:b=../../profiles/build/clang-linux-x86_64 -pr:h=../../profiles/host/clang-linux-x86_64 --build=missing
$ cd libs/engine/
$ conan create . -pr:b=../../profiles/build/clang-linux-x86_64 -pr:h=../../profiles/host/clang-linux-x86_64 --build=missing
$ cd ../../apps/app
$ conan install . -pr:b=../../profiles/build/clang-linux-x86_64 -pr:h=../../profiles/host/clang-linux-x86_64 --build=missing -s build_type=Release
$ cmake --preset conan-release
$ cmake --build build/Release
$ cd build/Release/
Run the build/Release/app on your Linux machine
```

### Build project with MinGW Dev Container

```sh
$ cd libs/log/
$ conan create . -pr:b=../../profiles/build/mingw-windows-x86_64 -pr:h=../../profiles/host/mingw-windows-x86_64 --build=missing
$ cd libs/engine/
$ conan create . -pr:b=../../profiles/build/mingw-windows-x86_64 -pr:h=../../profiles/host/mingw-windows-x86_64 --build=missing
$ cd ../../apps/app
$ conan install . -pr:b=../../profiles/build/mingw-windows-x86_64 -pr:h=../../profiles/host/mingw-windows-x86_64 --build=missing -s build_type=Release
$ cmake --preset conan-release
$ cmake --build build/Release
Run the build/Release/app.exe on your Windows machine
```
