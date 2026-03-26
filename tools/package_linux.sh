#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-linux-release}"

cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_D3D11_RENDERER=OFF \
  -DENABLE_D3D11VA=OFF \
  -DENABLE_DXVA2=OFF \
  -DENABLE_OPENGL_RENDERER=ON \
  -DENABLE_SDL_RENDERER=ON \
  -DENABLE_VAAPI=ON \
  -DENABLE_VIDEOTOOLBOX=OFF

cmake --build "${BUILD_DIR}" --parallel

cpack --config "${BUILD_DIR}/CPackConfig.cmake" -G DEB
cpack --config "${BUILD_DIR}/CPackConfig.cmake" -G TGZ

echo "Linux packages generated in ${BUILD_DIR}"
