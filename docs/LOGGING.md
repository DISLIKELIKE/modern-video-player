# 日志系统说明

## 当前状态

- 已重新启用 Quill 6.x，使用 **异步前端 + 双接收器**（ConsoleSink + RotatingFileSink）。
- 默认在 `logs/modern-video-player.log` 中保留 5 个 10 MB 文件，同时实时输出到终端。
- `USE_QUILL_LOGGING` 编译宏缺失或初始化失败时，自动回退到 stdout/stderr，不影响播放器流程。
- `DEBUG_MODE` 仍用于在编译期裁剪 `LOG_DEBUG`/`LOG_TRACE_*` 宏，运行期 `log_level` 继续控制双接收器的最小等级。

## 架构概览

```
LOG_* 宏 → Logger facade → LoggingConfigLoader → QuillBootstrapper
                                           ↘︎ StdoutFallback (权限/配置异常)
```

- `logger.h / logger.cpp`：封装所有日志入口，保证调用侧零改动。
- `LoggingConfigLoader`：依次读取 `config/logging.conf` → 环境变量 → 默认值，生成 `LoggingConfig`。
- `QuillBootstrapper`：启动 Quill 后端线程，注册 ConsoleSink 与 RotatingFileSink；任何异常均降级到 fallback。
- `StdoutFallback`：完全复制旧版 `[LEVEL][CATEGORY]` 输出格式，可在无 Quill 或目录不可写时持续使用。

## 运行时配置

### 配置文件

- 路径：`config/logging.conf`（相对当前工作目录）
- 示例：

```
log_dir=logs
max_file_size_mb=10
max_files=5
log_level=info
```

### 支持的键

| 键 | 说明 | 取值范围 | 默认值 |
|----|------|----------|--------|
| `log_dir` | 日志目录（可相对或绝对） | 有效路径 | `logs` |
| `max_file_size_mb` | 单文件上限（MB） | 1–1024 | 10 |
| `max_files` | 轮转保留文件数 | 1–50 | 5 |
| `log_level` | 最低记录等级 | `trace/debug/info/warning/error` | `info` |

缺失或非法值会被自动回退至表格中的默认值，并通过 `LOG_WARNING` 输出纠正信息。

### 环境变量覆盖

| 变量 | 对应键 | 备注 |
|------|--------|------|
| `MVP_LOG_CONFIG` | 配置文件绝对/相对路径 | 优先于默认 `config/logging.conf` |
| `MVP_LOG_DIR` | `log_dir` | 自动创建目录，失败则降级 |
| `MVP_LOG_LEVEL` | `log_level` | 不区分大小写 |
| `MVP_LOG_MAX_FILE_MB` | `max_file_size_mb` | 非数字自动警告 |
| `MVP_LOG_MAX_FILES` | `max_files` | 非数字自动警告 |

所有环境变量覆盖在 Quill 初始化前评估，确保容器化/CI 环境无需改动配置文件。

## 宏与分类

- `LOG_INFO/LOG_WARNING/LOG_ERROR`：始终可用。
- `LOG_DEBUG/LOG_TRACE_VIDEO/LOG_TRACE_AUDIO/LOG_TRACE_EVENT/LOG_TRACE_LOOP`：仅在 `DEBUG_MODE=ON` 且 `log_level` 允许时输出。
- 内部仍保留 `Logger::info/warning/error/debug` 供 C++ 代码直接调用，宏层会向消息中注入 `[CATEGORY]` 前缀。

## 故障与降级策略

| 场景 | 行为 |
|------|------|
| `USE_QUILL_LOGGING` 未定义 | 记录一次 `[INFO][LOGGER] Quill logging disabled...`，随后完全使用 stdout/stderr |
| 配置文件缺失 | 自动采用默认值并提示 `[INFO] Loading default logging config` |
| 目录不可写 / 磁盘满 | 输出 `[WARNING]`，跳过文件接收器，仅保留 ConsoleSink 或降级到 stdout |
| Quill 初始化抛出异常 | 输出 `[ERROR]` + 异常信息，停止 Quill backend，继续 stdout |

## 验证步骤

1. **准备构建**
   ```bash
   # 生成构建（默认开启 USE_QUILL_LOGGING 与 DEBUG_MODE）
   cmake -B build
   ```

2. **运行播放器并观察双通道**
   ```bash
   # 二进制位于 build/ 目录，示例视频自备
   ./build/modern-video-player sample.mp4
   ```

3. **验证轮转**
   - 将 `max_file_size_mb` 调小为 `1`，播放任意视频，确认 `logs/modern-video-player.log.*` 依次生成，数量受 `max_files` 限制。

4. **验证覆盖**
   ```bash
   # 仅向终端输出，跳过文件
   MVP_LOG_DIR=/root/readonly ./build/modern-video-player sample.mp4
   ```
   - 预期看到 `[WARNING][LOGGER] Cannot create log directory ...`，且后续只在控制台打印。

## 相关文件

- `include/logger.h` / `src/logger.cpp`
- `config/logging.conf`
- `docs/CHANGELOG.md`（问题 5，企业级日志改造）
- `docs/VERSION.md`（依赖状态与配置说明）
