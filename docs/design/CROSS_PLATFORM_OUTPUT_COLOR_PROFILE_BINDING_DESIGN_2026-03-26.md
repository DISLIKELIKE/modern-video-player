# Cross-Platform Output Color Profile Binding Design
Date: 2026-03-26
Scope: Phase 8 (`CP-802 ~ CP-805`)

## Goal
- Convert OpenGL output color management from a static manual LUT hook into a runtime-bound display/output policy.
- Keep manual `.cube` override intact while adding ICC/profile-driven automatic behavior.
- Expose enough machine-readable state that future CI and the remaining `CP-801` work can build on the same runtime model.

## Design Principles
1. Manual override beats automatic discovery.
2. Display binding is a runtime state, not a startup-only assumption.
3. Output diagnostics must describe the active source, not only whether a texture exists.
4. `CP-801` should consume the same display-binding snapshot instead of inventing a second path.

## Source Selection Order
1. Manual `.cube` (`MVP_OPENGL_3DLUT_FILE` / `--opengl-3dlut`)
2. Manual ICC profile (`MVP_OPENGL_ICC_PROFILE_FILE` / `--opengl-icc-profile`)
3. Auto ICC from current display (`MVP_OPENGL_AUTO_ICC=1` / `--opengl-auto-icc`)
4. None

## ICC LUT Generation Model
- Accept RGB matrix/shaper monitor profiles first.
- Parse:
  - `rXYZ`, `gXYZ`, `bXYZ`
  - `rTRC`, `gTRC`, `bTRC`
  - `desc` / `mluc`
- Generate sampled 3D LUT in encoded RGB domain so it can plug into the existing OpenGL output stage.
- Initial LUT size: `17^3`.

## Display Binding Model
### Runtime state
- SDL display index
- SDL display name
- Windows monitor device name
- current ICC profile path (if selected / auto-resolved)

### Dirty events
- move
- display change
- resize / restore / maximize
- fullscreen transition

### Reload behavior
- Mark binding dirty on UI/event thread.
- Rebuild LUT on render thread after GL context is current.
- Avoid doing GL texture operations on the event thread.

## Diagnostics Model
### Renderer diagnostics / performance-log
- output display index / name / device name
- ICC availability / source / path / description
- LUT source / size / reload count / error
- binding error string

### CLI surface
- `--opengl-output-color-check` for manual cube
- `--opengl-output-color-icc-check` for auto ICC
- `--opengl-diagnostics` for static probe + display/ICC snapshot

## Deferred Part
- `CP-801` remains the real HDR presentation step:
  - DXGI present bridge
  - `SetColorSpace1`
  - `SetHDRMetaData`
- The key point is that it should attach to the same display-binding/runtime output model landed here.
