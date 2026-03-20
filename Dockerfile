# BUILD STAGE
FROM debian:trixie-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    make git gcc g++ build-essential cmake wget python3 python3-pip \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install Conan 2.x
RUN pip install conan --break-system-packages

WORKDIR /build_src
COPY conanfile.txt .
RUN conan profile detect --force
RUN mkdir -p build && cd build && \
    conan install .. --output-folder=. --build=missing

COPY . .
RUN cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc)

# RUNTIME STAGE
FROM debian:trixie-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build_src/build/backend-cockfighting-api .
RUN chmod +x /app/backend-cockfighting-api

CMD ["/app/backend-cockfighting-api"]
