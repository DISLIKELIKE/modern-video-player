# Cross-Platform Phase 8 Output Color Closure Plan
Date: 2026-03-26
Status: Landed (`CP-801 ~ CP-805` done)

## Scope
- Land ICC/profile-driven output LUT generation.
- Land per-display output binding + reload strategy.
- Land runtime diagnostics + dedicated regression coverage.
- Close `CP-801` on the real D3D11 DXGI present path.

## Dependency-Ordered Steps
1. Add reusable output color helper.
   - output: `.cube` parser + ICC profile -> 3D LUT generator
   - dependency: none
2. Add OpenGL runtime display binding.
   - output: current display index/name/device + ICC path resolution
   - dependency: step 1
3. Add render-thread LUT rebuild flow.
   - output: dirty-mark + reload on display/source changes
   - dependency: step 2
4. Extend diagnostics + CLI.
   - output: performance-log / opengl-diagnostics / dedicated ICC check
   - dependency: step 3
5. Extend gate coverage.
   - output: manual cube + auto ICC regressions in OpenGL gate
   - dependency: step 4
6. Update records and local validation report.
   - dependency: steps 1-5

## Landed Result
- Step 1: done
- Step 2: done
- Step 3: done
- Step 4: done
- Step 5: done
- Step 6: done

## Final Closure
- `CP-801` landed with:
  - DXGI HDR output probe binding
  - D3D11 HDR present decision/runtime diagnostics
  - `--d3d11-hdr-output-check`
  - D3D11 present timing counters surfaced through `--performance-log-check`
