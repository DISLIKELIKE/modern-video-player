# Design Docs

Use [ARCHITECTURE.md](./ARCHITECTURE.md) as the current architecture source of truth.

## Current Documents

| Document | Purpose |
| --- | --- |
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Current source-level architecture. |
| [ARCHITECTURE_REFACTOR_2026-03-06.md](./ARCHITECTURE_REFACTOR_2026-03-06.md) | Short status note for the completed architecture migration. |
| [CROSS_PLATFORM_RENDER_INPUT_OVERLAY_BOUNDARY_DESIGN_2026-03-26.md](./CROSS_PLATFORM_RENDER_INPUT_OVERLAY_BOUNDARY_DESIGN_2026-03-26.md) | Renderer/input boundary design record. |
| [CROSS_PLATFORM_SUBTITLE_FONT_PLATFORM_CLOSURE_DESIGN_2026-03-26.md](./CROSS_PLATFORM_SUBTITLE_FONT_PLATFORM_CLOSURE_DESIGN_2026-03-26.md) | Subtitle font platform closure design record. |
| [CROSS_PLATFORM_SUBTITLE_LIBASS_AND_LIVE_PACKET_PROBE_DESIGN_2026-03-26.md](./CROSS_PLATFORM_SUBTITLE_LIBASS_AND_LIVE_PACKET_PROBE_DESIGN_2026-03-26.md) | libass and live subtitle probe design record. |
| [CROSS_PLATFORM_HDR_PRESENT_OBSERVABILITY_AND_CI_DESIGN_2026-03-26.md](./CROSS_PLATFORM_HDR_PRESENT_OBSERVABILITY_AND_CI_DESIGN_2026-03-26.md) | HDR/output observability design record. |

## Historical Design Records

The many `CROSS_PLATFORM_*` and `CROSS_PLATFORM_VULKAN_*` files are historical design records for already-landed increments. They are useful for traceability, but they are not the best place to learn the current architecture.

## Update Rule

- Update `ARCHITECTURE.md` when source structure or ownership changes.
- Keep new design notes focused on decisions that are not obvious from source.
- Do not add a new long design file for every small fix.
