# BUILD STAGE
FROM debian:trixie-20260623-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    make git gcc g++ build-essential cmake wget python3 python3-pip \
    ca-certificates uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Conan 2.x
RUN pip install conan --break-system-packages

WORKDIR /build_src
COPY conanfile.txt .

RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    conan profile detect --force && \
    mkdir -p build && cd build && \
    conan install .. --output-folder=. --build=missing 2>&1 | tee /tmp/conan.log; \
    if grep -q "Error in source" /tmp/conan.log 2>/dev/null; then \
        echo "Patching conandata.yml for util-linux..."; \
        CONANDATA=$(find /root/.conan2 -name "conandata.yml" -path "*util-linux*" 2>/dev/null | head -1); \
        if [ -n "$CONANDATA" ]; then \
            sed -i 's|mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2\.39/util-linux-2\.39\.2\.tar\.xz|github.com/util-linux/util-linux/archive/refs/tags/v2.39.2.tar.gz|g' "$CONANDATA"; \
            echo "Patched $CONANDATA"; \
            conan install .. --output-folder=. --build=missing; \
        else \
            echo "Could not find conandata.yml, trying direct download..."; \
            wget -q -O /root/.conan2/cache/util-linux-2.39.2.tar.gz \
                "https://github.com/util-linux/util-linux/archive/refs/tags/v2.39.2.tar.gz" 2>&1 && \
            conan install .. --output-folder=. --build=missing || \
            echo "Still failing, trying with --build=never for util-linux..."; \
            conan install .. --output-folder=. --build=missing --build-policy="util-linux-libuuid/2.39.2:never" 2>&1 || true; \
        fi; \
    fi

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
