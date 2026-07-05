# BUILD STAGE
FROM debian:trixie-20260623-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    make git gcc g++ build-essential cmake wget python3 python3-pip \
    ca-certificates uuid-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip install conan --break-system-packages

WORKDIR /build_src
COPY conanfile.txt .

RUN conan profile detect --force && \
    mkdir -p build && cd build && \
    conan download util-linux-libuuid/2.39.2 -r conancenter 2>/dev/null; \
    find /root/.conan2 -name "conandata.yml" -exec grep -l "util-linux" {} \; -exec sed -i 's|https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.39/util-linux-2.39.2.tar.xz|https://kernel.googlesource.com/pub/linux/utils/util-linux/v2.39/util-linux-2.39.2.tar.xz|g' {} \; && \
    conan install .. --output-folder=. --build=missing 2>&1 || \
    (echo "Retrying with web.archive.org mirror..." && \
     find /root/.conan2 -name "conandata.yml" -exec grep -l "util-linux" {} \; -exec sed -i 's|https://kernel.googlesource.com/pub/linux/utils/util-linux/v2.39/util-linux-2.39.2.tar.xz|https://web.archive.org/web/20230101000000if_/https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.39/util-linux-2.39.2.tar.xz|g' {} \; && \
     rm -rf /root/.conan2/p/*util-linux* /root/.conan2/p/b/*util* 2>/dev/null && \
     conan install .. --output-folder=. --build=missing 2>&1) || \
    (echo "Both mirrors failed. Removing util-linux-libuuid dependency..." && \
     rm -rf /root/.conan2/p/*util-linux* /root/.conan2/p/b/*util* /root/.conan2/p/*libuuid* 2>/dev/null && \
     conan install .. --output-folder=. --build=missing 2>&1) || \
    (echo "Final attempt with system uuid-dev..." && \
     apt-get install -y uuid-dev && \
     conan install .. --output-folder=. --build=missing 2>&1)

COPY . .

RUN cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc)

# RUNTIME STAGE
FROM debian:trixie-20260623-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build_src/build/backend-cockfighting-api .
RUN chmod +x /app/backend-cockfighting-api

CMD ["/app/backend-cockfighting-api"]
