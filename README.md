# Readme

## Quick start

Build the base dev container with **Docker**

```sh
$ docker build --no-cache -t base:latest -f .devcontainer/base/Dockerfile .
```

Then open the project with **Visual Studio Code** and then reopen with in container:

```
Ctrl+Shift+P > Dev Containers: Reopen in Container
```

And build the project with

```
Ctrl+Shift+B
```

Now run the compiled binary `apps/app/build/Release/app` or `apps/app/build/Release/app.exe` on your target machine.

## Manual build steps

### Build dev containers

```sh
$ docker build --no-cache -t base:latest -f .devcontainer/base/Dockerfile .
$ docker build --no-cache -t gcc:latest -f .devcontainer/gcc/Dockerfile .
$ docker build --no-cache -t clang:latest -f .devcontainer/clang/Dockerfile .
$ docker build --no-cache -t mingw-w64:latest -f .devcontainer/mingw-w64/Dockerfile .
```

### Build with GCC Dev Container for Linux

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

### Build with Clang Dev Container for Linux

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

### Build with Mingw-w64 Dev Container for Windows

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
