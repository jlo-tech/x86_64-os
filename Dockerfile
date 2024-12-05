# Dockerize the build system
FROM ubuntu:latest

RUN apt update && apt install -y
RUN apt install make xorriso nasm gcc grub2 grub2-common -y

RUN mkdir -p /x86_64-OS
