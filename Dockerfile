# BUILD STAGE
FROM debian:trixie-20260824-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    make git gcc g++ build-essential cmake wget python3 python3-pip \
    ca-certificates uuid-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip install conan --break-system-packages

WORKDIR /build_src
COPY conanfile.txt .

# ConanCenter removeu util-linux-libuuid/2.39.2 e o boost/1.83.0 quebra com
# Conan >= 2.3x: patch na receita baixada (uuid vem do uuid-dev do sistema,
# via shim CMake abaixo) e with_boost=False no conanfile.txt.
RUN conan profile detect --force && \
    conan download drogon/1.9.13 -r conancenter && \
    find /root/.conan2 -name "conanfile.py" -path "*drogon*" -exec sed -i 's|self.requires("util-linux-libuuid/2.39.2")|pass  # system uuid-dev|' {} \; && \
    mkdir -p /usr/local/lib/cmake/UUID && \
    printf 'find_library(UUID_LIBRARY NAMES uuid PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib REQUIRED)\nif(NOT TARGET UUID_lib)\n  add_library(UUID_lib UNKNOWN IMPORTED)\n  set_target_properties(UUID_lib PROPERTIES IMPORTED_LOCATION "${UUID_LIBRARY}")\nendif()\n' > /usr/local/lib/cmake/UUID/UUIDConfig.cmake && \
    printf '#include <uuid/uuid.h>\n' > /usr/local/include/uuid.h && \
    mkdir -p build && cd build && \
    conan install .. --output-folder=. --build=missing

COPY . .

RUN cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc)

# RUNTIME STAGE
FROM debian:trixie-20260824-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build_src/build/backend-cockfighting-api .
RUN chmod +x /app/backend-cockfighting-api

CMD ["/app/backend-cockfighting-api"]
