# Cross-platform Vulkan frame upload YUV path design (2026-03-27)

## 1. Goal
Deliver `VK-007` baseline frame upload path so Vulkan backend can display decoded video frames with at least one direct YUV path.

## 2. Scope
- In scope:
  - direct-frame acceptance for `YUV420P` / `NV12`
  - CPU conversion to RGBA (`swscale`)
  - host-visible staging buffer upload
  - command recording path: clear + optional frame copy + present
- Out of scope:
  - zero-copy import path
  - pacing optimization and advanced sync tuning (`VK-008`)
  - Vulkan diagnostics CLI (`VK-010`)

## 3. Data Flow
1. Decoder outputs software frame (`YUV420P` or `NV12`).
2. `renderFrame()` ensures `swscale` context and converts frame to RGBA in swapchain-target resolution.
3. RGBA payload is stored in renderer state (`frame_upload_*`) under mutex.
4. `present()` snapshots payload, ensures staging buffer, then `map -> memcpy -> unmap`.
5. `recordPresentCommandBuffer()` executes:
   - image layout transition to transfer dst
   - clear color
   - optional `vkCmdCopyBufferToImage` from staging payload
   - transition to present layout
6. queue submit and present stay on existing `VK-006` frame sync contract.

## 4. Threading and Safety
- `renderFrame()` and `present()` share upload payload via `frame_upload_mutex`.
- Vulkan resource ownership remains on renderer lifecycle:
  - upload resources released in `close()` via `destroyFrameUploadResources`.
- On upload staging/map failure, frame upload is skipped and clear-only present still proceeds, preserving fallback behavior.

## 5. Compatibility and Fallback
- Existing startup strategy/fallback contract is unchanged:
  - `Vulkan -> OpenGL -> SoftwareSDL` (Linux-first target)
- Windows regression path remains unaffected because Vulkan build switch is Linux-first gated in current CMake policy.
