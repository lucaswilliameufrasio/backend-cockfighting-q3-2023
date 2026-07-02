# BUILD STAGE
FROM debian:trixie-20260623-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    make git gcc g++ build-essential cmake wget python3 python3-pip \
    ca-certificates libuuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Conan 2.x
RUN pip install conan --break-system-packages

WORKDIR /build_src
COPY conanfile.txt .

RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    conan profile detect --force && \
    mkdir -p build && cd build && \
    conan install .. --output-folder=. --build=missing

COPY . .

RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    cd build && \
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
