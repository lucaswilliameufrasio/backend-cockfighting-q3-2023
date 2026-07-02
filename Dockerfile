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

# Download recipe, patch URL, then install
RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    conan profile detect --force && \
    mkdir -p build && cd build && \
    conan install .. --output-folder=. --build=missing 2>&1 || true; \
    CONANDATA=$(find /root/.conan2 -name "conandata.yml" -exec grep -l "util-linux" {} \; 2>/dev/null | head -1); \
    if [ -n "$CONANDATA" ]; then \
        echo "Found conandata.yml at $CONANDATA, patching URL and SHA256..."; \
        sed -i 's|https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/v2.39/util-linux-2.39.2.tar.xz|https://github.com/util-linux/util-linux/archive/refs/tags/v2.39.2.tar.gz|g' "$CONANDATA"; \
        sed -i 's|87abdfaa8e490f8be6dde976f7c80b9b5ff9f301e1b67e3899e1f05a59a1531f|7a46eed743a84cc10108291237e06d8a524cbc8c07f60047667b84b95b01e267|g' "$CONANDATA"; \
        rm -rf /root/.conan2/p/*util-linux* /root/.conan2/p/b/*util* 2>/dev/null; \
        conan install .. --output-folder=. --build=missing; \
    else \
        echo "Trying with system uuid-dev..."; \
        conan install .. --output-folder=. --build=missing -c "tools.system.package_manager:mode=install" -c "tools.system.package_manager:tool=apt-get" 2>&1 || \
        echo "ERROR: util-linux source download still failing. Manual intervention needed."; \
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
