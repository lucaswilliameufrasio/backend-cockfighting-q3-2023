# BUILD STAGE
FROM debian:trixie-20260623-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    make git gcc g++ build-essential cmake wget python3 python3-pip \
    ca-certificates uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Conan 2.x
RUN pip install conan --break-system-packages

# Create and export a local util-linux-libuuid that wraps system libuuid
RUN mkdir -p /tmp/uuid-pkg && cd /tmp/uuid-pkg && \
cat > conanfile.py << 'CONANEOF'
from conan import ConanFile
class SysLibUuid(ConanFile):
    name = "util-linux-libuuid"
    version = "2.39.2"
    package_type = "static-library"
    settings = "os", "arch", "compiler", "build_type"
    def requirements(self):
        pass
    def package_info(self):
        self.cpp_info.libs = ["uuid"]
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []
CONANEOF
conan create . 2>&1 || conan export . 2>&1

WORKDIR /build_src
COPY conanfile.txt .

RUN --mount=type=cache,target=/root/.conan2,sharing=locked \
    conan profile detect --force && \
    mkdir -p build && cd build && \
    conan install .. --output-folder=. --build=missing 2>&1 || \
    conan install .. --output-folder=. --build="util-linux-libuuid/*:never" 2>&1

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
