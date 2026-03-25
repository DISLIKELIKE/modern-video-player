# PLAYERCORE_DAY33_OPENGL_PRESENT_PACING_AND_AMD_STUTTER

## Problem
- On the user's AMD machine, `MVP_RENDERER_BACKEND=opengl` playback showed visible stutter even though decode, queue, and copy-back counters did not indicate a hard throughput collapse.
- The reported log showed OpenGL native D3D11 interop staying active, no copy-back fallback, and frame queues staying near full, which meant the issue was not a simple decoder failure.

## Root Cause
1. OpenGL was the only renderer whose `present()` call returned before the frame was actually shown on screen.
2. `PlayerCore` advanced the video clock immediately after submit, while the real OpenGL swap happened later on the renderer thread.
3. The OpenGL renderer only kept one pending frame slot, so a slower render thread could silently replace queued frames without feeding that back into scheduler pacing.
4. OpenGL forced `swap interval=0`, unlike the D3D11 renderer which presents with synchronized pacing.
5. The native interop path also called `ID3D11DeviceContext::Flush()` every frame, which increases driver serialization risk on sensitive stacks.

## Solution
- Changed OpenGL `present()` to wait until the render thread has actually displayed the submitted frame.
- Added per-frame submission/presentation IDs so the scheduler no longer treats an asynchronously queued frame as already visible.
- Switched OpenGL startup to prefer `swap interval=1`, with fallback to `0` only if enabling paced presentation fails.
- Removed the per-frame `Flush()` call from the native D3D11 interop update path.

## Expected Effect
- Playback cadence is now gated by real display completion instead of fire-and-forget submit timing.
- Visible stutter caused by render-thread lag or pending-frame replacement should be reduced substantially.
- OpenGL presentation timing is now closer to the D3D11 path.

## Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500`

## Notes
- Local validation on this workspace used an NVIDIA machine, so the exact AMD driver behavior could not be reproduced here.
- The local runs completed with `result=PASS`, native and copy-back OpenGL paths both remained functional, and no `OpenGL present wait timed out` warning was emitted.
- Audio device initialization is currently unavailable in this shell environment, so the automated checks ran in video-only fallback mode.
