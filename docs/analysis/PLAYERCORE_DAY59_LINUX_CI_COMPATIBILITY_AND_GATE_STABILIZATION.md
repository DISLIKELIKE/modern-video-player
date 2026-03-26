# PLAYERCORE Day59: Linux CI compatibility and gate stabilization

Date: 2026-03-26  
Status: Done (local implementation/validation landed, Linux CI green proof pending push)

## 1. Problem
- Cross-platform task matrix showed `CP-001 ~ CP-905` as done, but latest CI gate still failed.
- Linux lane failed to compile due FFmpeg API and libass include incompatibilities.
- Windows lane failed gate/packaging when probe media and plugin target outputs were missing in CI workspace.

## 2. Root Cause
1. Audio channel layout code depended on a single FFmpeg API generation (`AVChannelLayout` or legacy fields), not both.
2. `src/subtitle/libass_probe.cpp` included `<libass/ass.h>` only, while Ubuntu runner commonly exposes `<ass/ass.h>`.
3. OpenGL color enum used `None`, which is vulnerable to Linux/X11 macro collisions.
4. CI workflow assumed probe media exists in repo checkout and built only `modern-video-player`, while package install also requires `sample_logger_plugin`.

## 3. Solution
- Added FFmpeg channel layout compatibility helper:
  - new file `include/media/ffmpeg_channel_layout_compat.h`
  - explicit preprocessor split for new/legacy FFmpeg channel APIs
- Completed `PlayerCore` audio resampler compatibility migration:
  - removed direct `AVChannelLayout` state from runtime reinit path
  - added channel/layout scalar state (`uint64_t` + channel count)
  - switched resampler init path by swresample version (`swr_alloc_set_opts2` vs `swr_alloc_set_opts`)
- Fixed Linux compile blockers:
  - `libass` include fallback (`ass/ass.h` first, `libass/ass.h` fallback)
  - renamed OpenGL gamut enum value from `None` to `Disabled`
- Hardened CI workflow:
  - auto-generate probe media in Windows/Linux jobs when absent
  - build both `modern-video-player` and `sample_logger_plugin` targets

## 4. Outcome
- Local Windows Release build now passes with plugin target included.
- Cross-platform gate workflow definition is now deterministic on probe media presence and plugin build output.
- Linux compile compatibility paths for FFmpeg/libass are implemented in code.

## 5. Remaining Gap
- Linux runtime/compile PASS evidence still needs a real Linux runner execution after these local changes are pushed.
