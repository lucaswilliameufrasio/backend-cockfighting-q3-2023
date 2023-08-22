FROM debian:12-slim

COPY . .

RUN apt-get update && apt-get install make git gcc g++ build-essential cmake wget -y

RUN mkdir -p conanio && wget https://github.com/conan-io/conan/releases/latest/download/conan-linux-64.tar.gz && tar -xvf conan-linux-64.tar.gz -C conanio

RUN ln -s /conanio/conan /usr/bin/conan
