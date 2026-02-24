# Modern Video Player 企业级播放器需求

## 引言

Modern Video Player 需要参考 MPC-HC 级别的企业标准，提供模块化播放内核、插件式扩展能力以及与现有 C++17/FFmpeg/SDL2/Quill 技术栈兼容的跨平台基础设施。本文档使用 EARS 模板定义需求，确保 UI 与播放内核解耦，并覆盖企业级监控、配置、测试与资源生命周期管理。

## 术语表

- **Modern Video Player Core**：播放器的核心引擎，负责解复用、解码、同步、渲染等流程。
- **Filter Graph**：可插拔的处理链，包含字幕、后期处理、音频效果等 Filter 插件。
- **Message Bus**：统一消息/事件系统，桥接 UI 控件、热键、播放内核与外部监控。
- **Media Session**：一次媒体播放会话，涵盖输入源、状态机、资源生命周期。
- **Telemetry Service**：收集播放统计、错误上报与结构化日志的组件。
- **Config Service**：读取/写入 JSON 或 YAML 配置文件的中心化配置层。

## 需求

### R1 模块化播放核心

**用户故事：** 作为播放器平台工程师，我希望播放核心以 Demux、解码、同步、渲染、控制等模块化组件存在，从而在不影响 UI 的情况下升级或替换核心能力。

#### 验收标准

1. WHEN Modern Video Player Core 加载媒体会话，THE Modern Video Player Core SHALL 初始化 Demux、Video Decode、Audio Decode、AV Sync、Render、Control 等独立实例。
2. WHILE Modern Video Player Core 运行，THE UI 层 SHALL 仅通过已定义的 Core API 调用播放命令和读取状态。
3. IF 任一模块发生崩溃，THE Modern Video Player Core SHALL 将异常上报给 Message Bus 并保持其他模块隔离运行。

### R2 多线程流水线与调度

**用户故事：** 作为性能工程师，我希望播放内核拥有 IO、Demux、Decode、AV Sync、Render/Audio Output 等多线程流水线以及可扩展的任务调度，以满足高分辨率、低延迟播放需求。

#### 验收标准

1. WHEN Modern Video Player Core 启动，THE Task Scheduler SHALL 为 IO、Demux、Decode、AV Sync、Render、Audio Output 分配独立线程或线程池队列。
2. WHILE 多线程流水线运行，THE Task Scheduler SHALL 支持动态调整线程优先级与队列长度以满足可配置性能目标。
3. IF 任一流水线线程阻塞超过可配置阈值，THE Task Scheduler SHALL 触发回退策略并向 Telemetry Service 发出性能事件。

### R3 插件式 Filter Graph

**用户故事：** 作为扩展开发者，我希望通过插件式 Filter Graph 注入字幕、后期处理或音频效果，以便根据业务场景扩展媒体处理能力。

#### 验收标准

1. WHEN Filter 插件被注册，THE Filter Graph SHALL 在媒体流进入渲染前将插件节点插入处理链。
2. WHILE Filter Graph 执行，THE Modern Video Player Core SHALL 支持动态启用、禁用或重排插件并维持线程安全。
3. IF 插件抛出错误，THE Filter Graph SHALL 隔离异常节点、记录 Telemetry，并继续让其他节点运行。

### R4 统一消息总线与事件系统

**用户故事：** 作为交互工程师，我希望 UI 控件、热键与播放内核通过统一消息总线通信，从而实现解耦和一致的事件处理。

#### 验收标准

1. WHEN UI 触发 Play、Pause、Seek、Rate 或 Loop 命令，THE Message Bus SHALL 将事件路由到 Control 模块并带上时间戳与上下文。
2. WHILE 播放内核状态更新，THE Message Bus SHALL 发布状态事件供 UI、Telemetry 与脚本订阅。
3. IF 外设热键事件到达，THE Message Bus SHALL 将事件映射为标准化命令并执行去抖动逻辑。

### R5 状态管理与媒体会话生命周期

**用户故事：** 作为播放器 QA，我需要明确的 Play/Pause/Stop/Seek/Rate/Loop 状态管理以及媒体会话生命周期，确保资源按预期申请与释放。

#### 验收标准

1. WHEN 新媒体会话创建，THE Media Session Manager SHALL 记录媒体源、初始状态和资源句柄并进入 Idle 状态。
2. WHILE 会话处于 Play、Pause、Seek 或 Loop 状态，THE Media Session Manager SHALL 保持音视频同步上下文和 Buffer 引用的有效性。
3. IF 会话终止或切换，THE Media Session Manager SHALL 释放 FFmpeg、SDL2、Filter Graph 与调度资源并记录 Telemetry。

### R6 监控与 Telemetry

**用户故事：** 作为运营工程师，我希望获得播放统计、错误上报与日志通道，以便在企业环境中审计播放器行为。

#### 验收标准

1. WHEN 播放开始、暂停、错误或结束事件发生，THE Telemetry Service SHALL 写入结构化记录并支持批量传输。
2. WHILE 播放进行，THE Telemetry Service SHALL 暴露可订阅的帧率、缓冲、丢帧与 CPU/GPU 利用率指标。
3. IF 发生未捕获异常，THE Telemetry Service SHALL 捕获堆栈、模块 ID 与最近的 Message Bus 事件并输出到配置的日志通道。

### R7 配置中心

**用户故事：** 作为部署工程师，我需要一个支持 JSON 和 YAML 的配置中心，以集中管理播放参数、线程策略与插件设置。

#### 验收标准

1. WHEN 播放器启动，THE Config Service SHALL 加载 JSON 或 YAML 配置并验证 Schema。
2. WHILE 播放器运行，THE Config Service SHALL 支持热更新配置并通过 Message Bus 推送变更。
3. IF 配置验证失败，THE Config Service SHALL 回退到最近一次有效配置并发出 Telemetry 告警。

### R8 可测试性

**用户故事：** 作为测试自动化工程师，我需要通过接口和模拟器驱动播放核心，以编写回归测试和性能基准。

#### 验收标准

1. WHEN 测试套件运行，THE Modern Video Player Core SHALL 提供 API 以注入模拟媒体源、事件和过滤器。
2. WHILE 测试模拟进行，THE Core SHALL 允许替换真实 IO、解码器或渲染器为 Mock 组件。
3. IF 测试断言失败，THE Core SHALL 输出可重放的日志、配置快照与调度时间线。

### R9 跨平台适配层

**用户故事：** 作为跨平台负责人，我需要 Win/macOS/Linux 共享逻辑，但能按平台差异封装窗口、输入与音频输出。

#### 验收标准

1. WHEN 平台抽象层初始化，THE 系统 SHALL 检测操作系统并实例化对应 SDL2、音频与窗口适配器。
2. WHILE 播放器运行在任意支持平台，THE 平台层 SHALL 暴露统一接口给 UI 与 Core。
3. IF 平台特定功能缺失，THE 适配层 SHALL 提供能力检测并记录 Telemetry，而不是导致崩溃。

### R10 技术栈兼容性

**用户故事：** 作为核心开发者，我需要方案兼容现有 C++17 标准、FFmpeg 媒体栈、SDL2 渲染与 Quill 日志库，以降低迁移成本。

#### 验收标准

1. WHEN 构建系统生成产物，THE Toolchain SHALL 使用 C++17 编译选项并对 FFmpeg、SDL2、Quill 进行链接检查。
2. WHILE Core 运行，THE 系统 SHALL 通过 FFmpeg 完成媒体解复用与解码，通过 SDL2 完成窗口与输入管理。
3. IF 日志需要输出，THE Core SHALL 使用 Quill 提供的异步日志通道以保证线程安全。
