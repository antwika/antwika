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

> You may need to delete the "build" directory when switching between dev containers...

Build the project with

```
Ctrl+Shift+B
```

Now run the compiled binary `build/apps/app/app` or `build/apps/app/app.exe` on your target machine.

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

### Build steps (same as when doing Ctrl+Shift+B in VSCode)

Launch one of the dev-containers

```sh
$ rm -Rf build
$ conan install . -of build -pr:b=./profiles/build/${CONAN_PROFILE} -pr:h=./profiles/host/${CONAN_PROFILE} --build=missing -s build_type=Release
$ cmake --preset conan-release
$ cmake --build build
$ ctest --test-dir build
```
