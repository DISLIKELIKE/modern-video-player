# Day 4 执行版（渲染后端与零拷贝链路专项）

日期：2026-03-13  
目标：吃透当前渲染后端差异，明确“硬解但非零拷贝”的关键原因与改造方向。

---

## 1. 今日执行安排（8 小时）

1. `09:00-09:30` 回看 Day3 性能瓶颈结论，锁定今天只看渲染与硬解交界。
2. `09:30-10:30` 读 `renderer_factory`，确认后端选择与回退逻辑。
3. `10:30-11:20` 读 `D3D11VideoRenderer` 与 `SdlVideoRenderer`，确认两者是否共享显示路径。
4. `11:20-12:00` 读 `Display` 渲染线程，确认数据复制与纹理上传路径。
5. `13:30-14:30` 读 `prepareVideoOutputFrame()`，确认硬件帧回拷与像素格式转换条件。
6. `14:30-15:20` 输出当前“解码 -> 渲染”数据流图（标注 copy 点）。
7. `15:20-16:00` 形成零拷贝改造草案（分短中长期）。
8. `16:00-17:20` 产出 Day4 结论：后端能力矩阵 + 改造优先级。

---

## 2. 明日必须产出

1. 一张“当前渲染后端能力矩阵”（SDL / D3D11 / OpenGL）。
2. 一张“零拷贝差距图”（现状 copy 点 vs 目标路径）。
3. 一份技术决策清单（保守方案 / 进取方案 / 风险）。

---

## 3. 重点代码定位（渲染专题）

- `src/render/renderer_factory.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/display.cpp`
- `src/core/player_core.cpp`（`prepareVideoOutputFrame` / `convertVideoFrameToYuv420`）

---

## 4. Day4 验收标准

- 能解释当前为什么属于“硬解 + 回拷 + 上传”，不是零拷贝。
- 能解释 D3D11 渲染器与 SDL 渲染路径的关系。
- 能给出 3 条零拷贝改造路线并说明代价与风险。

