# Antwika

## Quick start

Open the project with **Visual Studio Code** and then reopen in container:

```
Ctrl+Shift+P > Dev Containers: Reopen in Container
```

> You may need to delete the "build" directory when switching between dev containers...

Build the project with

```
Ctrl+Shift+B
```

Now run the compiled binary `build/apps/app/app` or `build/apps/app/app.exe` on your target machine.

## Optional: Use a locally built antwika-dev-base dev containers

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

Now VSCode will use the locally built antwika-dev-base container 
