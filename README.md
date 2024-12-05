# x86_64-os

## Building (with Docker)

### Build the container

To do this and to install the necessary tools run:

```sh
docker build --platform linux/x86_64 --tag osbuild .
```

### Build the operating system

First switch to the container:

```sh
docker run --rm -it -v $(pwd):/x86_64-OS:z osbuild
```

Then inside the container run:
```sh
make
```

It is then usually a good idea to run qemu outside the container with:
```sh
make run
```