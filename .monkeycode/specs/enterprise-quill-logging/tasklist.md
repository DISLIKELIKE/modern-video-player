# 需求实施计划 - Enterprise Quill Logging + MPC-HC 架构

---

# 第一部分：Enterprise Quill Logging 任务列表

- [x] 1. 创建日志配置文件加载器
  - [x] 1.1 创建 LoggingConfig 数据结构
    - 定义 `struct LoggingConfig` 包含 log_dir, max_file_size_bytes, max_files, log_level, enabled 字段
    - 参考 design.md 数据模型部分

  - [x] 1.2 实现 LoggingConfigLoader 类
    - 实现 `static LoggingConfig load(const std::string& path)` 函数
    - 实现配置文件解析（忽略空行和 # 注释）
    - 实现环境变量覆盖逻辑（MVP_LOG_DIR, MVP_LOG_LEVEL, MVP_LOG_MAX_FILE_MB, MVP_LOG_MAX_FILES）
    - 实现默认值应用和数值校验（1-1024 MB, 1-50 files）

  - [ ]* 1.3 编写 LoggingConfigLoader 单元测试
    - 测试默认值加载
    - 测试配置文件解析
    - 测试环境变量优先级
    - 测试非法数值裁剪

- [x] 2. 实现 QuillBootstrapper 启动器
  - [x] 2.1 创建 ConsoleSink 配置
    - 使用 quill::Frontend::create_or_get_sink 创建 stdout sink
    - 关闭彩色输出以兼容 Windows

  - [x] 2.2 实现 RotatingFileSink 配置
    - 根据 max_file_size_bytes 和 max_files 创建滚动文件 sink
    - 使用 "rotating" 作为稳定 sink 名称

  - [x] 2.3 实现 QuillBackend 启动逻辑
    - 设置 quill::BackendOptions（cpu_affinity, thread_params）
    - 注册双 sinks 到 logger
    - 实现 getLogger() 返回已注册的 logger

  - [x] 2.4 实现 fallback 机制
    - 当 Quill 启动失败时返回 nullptr
    - 在 Logger 中实现 stdout fallback 路径

  - [ ]* 2.5 编写 QuillBootstrapper 单元测试
    - 测试双 sink 注册
    - 测试 fallback 触发

- [x] 3. 修改现有 Logger 类以支持 Quill
  - [x] 3.1 更新 Logger::init() 实现
    - 调用 LoggingConfigLoader 加载配置
    - 调用 QuillBootstrapper 启动 Quill backend
    - 当 USE_QUILL_LOGGING 未定义时输出 INFO 消息

  - [x] 3.2 更新日志宏实现
    - 保持 LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG, LOG_TRACE_* 宏签名不变
    - 当 QUILL 可用时路由到 Quill logger
    - 当 DEBUG_MODE=OFF 时，LOG_TRACE_* 和 LOG_DEBUG 在编译期禁用

  - [x] 3.3 实现 Logger::shutdown()
    - 调用 QuillBootstrapper::stop() 停止 backend
    - 清理资源

  - [ ]* 3.4 验证现有代码兼容性
    - 编译现有播放器代码，确认无需修改调用方

- [x] 4. 创建配置文件模板
  - [x] 4.1 创建 config/logging.conf 示例文件
    - 包含 log_dir, max_file_size_mb, max_files, log_level 四个配置项
    - 添加注释说明每个配置的用途

  - [x] 4.2 确保 CMake 创建 logs 目录
    - 在构建时自动创建 logs 目录

- [x] 5. 检查点 - 编译验证
  - [x] 5.1 使用 USE_QUILL_LOGGING=ON 编译
  - [x] 5.2 使用 USE_QUILL_LOGGING=OFF 编译
  - [x] 5.3 确认两种模式都能正常工作

- [x] 6. 更新文档
  - [x] 6.1 创建或更新 docs/design/LOGGING.md
    - 描述 Quill 配置工作流
    - 说明默认值和 fallback 行为
    - 列出环境变量覆盖方式
    - 提供权限错误排查步骤

  - [x] 6.2 更新 docs/records/CHANGELOG.md
    - 添加 enterprise logging 功能条目

  - [x] 6.3 更新 docs/records/VERSION.md
    - 添加 Quill 依赖状态
    - 列出新的配置产物

---

# 第二部分：播放器架构迁移路线图

## 概述

本路线图描述如何将当前的单线程播放器逐步迁移到多线程架构，提升播放流畅度和资源利用率。

## 阶段划分

### 阶段 1：架构基础设施（优先级：高）

- [x] 1.1 创建线程安全任务队列（TaskQueue）
  - [x] 1.1.1 设计无锁队列（LockFreeQueue）或互斥锁队列
  - [x] 1.1.2 实现 enqueue/dequeue 操作
  - [x] 1.1.3 添加线程安全断言

- [x] 1.2 创建帧缓冲池（FramePool）
  - [x] 1.2.1 实现预分配的 AVFrame 池
  - [x] 1.2.2 实现帧获取/归还机制
  - [x] 1.2.3 实现池大小可配置

- [x] 1.3 创建解码线程基类
  - [x] 1.3.1 设计 DecoderThread 接口
  - [x] 1.3.2 实现线程启动/停止/暂停控制

### 阶段 2：播放管线重构（优先级：高）

- [ ] 2.1 分离视频解码线程
  - [ ] 2.1.1 创建独立的 VideoDecodeThread 类
  - [ ] 2.1.2 将视频解码逻辑从 playLoop 移入解码线程
  - [ ] 2.1.3 通过帧队列将解码帧传递给渲染

- [ ] 2.2 分离音频解码线程
  - [ ] 2.2.1 创建独立的 AudioDecodeThread 类
  - [ ] 2.2.2 实现音频帧队列
  - [ ] 2.2.3 与 AudioPlayer 集成

- [ ] 2.3 实现同步管理器（SyncManager）
  - [ ] 2.3.1 计算音频/视频 pts 差值
  - [ ] 2.3.2 实现同步策略（音频为主、视频为主、自由）
  - [ ] 2.3.3 实现丢帧/重复帧策略

- [ ] 2.4 重构主渲染循环
  - [ ] 2.4.1 简化 playLoop 为纯渲染循环
  - [ ] 2.4.2 从帧队列获取已解码帧
  - [ ] 2.4.3 处理 seek 操作时的队列清空

### 阶段 3：高级特性（优先级：中）

- [ ] 3.1 视频预解码机制
  - [ ] 3.1.1 实现预解码队列（提前 N 帧）
  - [ ] 3.1.2 配置预解码数量阈值
  - [ ] 3.1.3 动态调整预解码数量

- [ ] 3.2 音频缓冲管理
  - [ ] 3.2.1 实现音频帧预缓冲
  - [ ] 3.2.2 实现缓冲水位监控
  - [ ] 3.2.3 处理音频解码落后情况

- [ ] 3.3 硬件加速解码支持
  - [ ] 3.3.1 添加 NVDecode（CUDA）支持
  - [ ] 3.3.2 添加 D3D11VA 支持
  - [ ] 3.3.3 实现硬件帧格式转换

### 阶段 4：性能优化（优先级：低）

- [ ] 4.1 零拷贝帧传递
  - [ ] 4.1.1 使用指针传递而非拷贝
  - [ ] 4.1.2 优化帧队列传输

- [ ] 4.2 CPU 亲和性设置
  - [ ] 4.2.1 设置解码线程 CPU 亲和性
  - [ ] 4.2.2 设置渲染线程 CPU 亲和性

---

# 第三部分：企业级 MPC-HC 软件模块规划

## 模块总览

| 模块编号 | 模块名称 | 描述 | 优先级 |
|---------|---------|------|--------|
| 01 | 核心播放引擎 | 多线程解码、同步、渲染管线 | 高 |
| 02 | 滤镜系统 | 视频/音频滤镜链管理 | 高 |
| 03 | 视频渲染器 | 视频输出（EVR, D3D, OpenGL） | 高 |
| 04 | 音频渲染器 | 音频输出、混音、均衡器 | 高 |
| 05 | 字幕系统 | 内嵌/外置字幕渲染 | 中 |
| 06 | 播放列表 | 列表管理、排序、持久化 | 中 |
| 07 | 解码器管理 | 内部/外部解码器选择 | 高 |
| 08 | 文件格式支持 | 容器格式解析 | 高 |
| 09 | 流媒体支持 | 网络流播放（HLS, DASH） | 中 |
| 10 | 皮肤系统 | UI 主题/皮肤引擎 | 低 |
| 11 | 快捷键系统 | 可配置的热键绑定 | 低 |
| 12 | 设置系统 | 配置管理、持久化 | 中 |
| 13 | 插件系统 | 扩展点机制 | 低 |
| 14 | 视频增强 | 色彩调整、锐化 | 低 |

---

# 第四部分：各模块详细实施计划

## 模块 01：核心播放引擎

### 目标
实现企业级的多线程播放核心，支持并行解码、同步控制和平滑渲染。

### 任务拆分

- [x] 1. 创建线程安全帧队列（FrameQueue）
  - [x] 1.1 定义 FrameQueue 模板类
  - [x] 1.2 实现 push/pop 操作，带超时
  - [x] 1.3 实现 clear 操作（用于 seek）
  - [x] 1.4 实现队列大小限制

- [x] 2. 实现视频解码线程（VideoDecodeThread）
  - [x] 2.1 创建 VideoDecodeThread 类，继承自 std::thread
  - [x] 2.2 实现 run() 方法，包含解码循环
  - [x] 2.3 实现 start/pause/stop 控制方法
  - [x] 2.4 实现与 FramePool 集成

- [x] 3. 实现音频解码线程（AudioDecodeThread）
  - [x] 3.1 创建 AudioDecodeThread 类
  - [x] 3.2 实现音频帧解码逻辑
  - [x] 3.3 实现与 AudioPlayer 的数据传递

- [x] 4. 实现同步管理器（SyncManager）
  - [x] 4.1 定义同步模式枚举（AudioMaster, VideoMaster, Free）
  - [x] 4.2 实现 pts 计算和比较
  - [x] 4.3 实现帧等待/跳过策略
  - [x] 4.4 添加帧速率检测

- [x] 5. 重构 VideoPlayer 主类
  - [x] 5.1 添加解码线程成员变量
  - [x] 5.2 添加帧队列成员变量
  - [x] 5.3 实现播放/暂停/停止线程同步
  - [x] 5.4 实现 seek 时队列清空逻辑

### 里程碑
- 视频解码线程独立运行
- 音频解码线程独立运行
- 主循环仅负责渲染，延迟 < 5ms

---

## 模块 02：滤镜系统

### 目标
实现可插拔的滤镜架构，支持视频和音频滤镜链。

### 任务拆分

- [x] 1. 定义滤镜基类（FilterBase）
  - [x] 1.1 定义 IFilter 接口
  - [x] 1.2 定义 FilterType 枚举（Video, Audio）
  - [x] 1.3 定义输入/输出pin接口

- [x] 2. 实现视频滤镜链（VideoFilterChain）
  - [x] 2.1 创建 VideoFilterChain 管理类
  - [x] 2.2 实现 addFilter/removeFilter 方法
  - [x] 2.3 实现滤镜顺序管理
  - [x] 2.4 实现链式处理方法

- [x] 3. 实现音频滤镜链（AudioFilterChain）
  - [x] 3.1 创建 AudioFilterChain 管理类
  - [x] 3.2 实现音频帧处理接口

- [x] 4. 内置滤镜实现
  - [x] 4.1 实现亮度/对比度滤镜
  - [x] 4.2 实现色彩平衡滤镜
  - [x] 4.3 实现音量/平衡滤镜

### 里程碑
- 滤镜可动态添加/移除
- 滤镜链正确处理帧数据

---

## 模块 03：视频渲染器

### 目标
实现多后端视频渲染支持，包括 D3D11、OpenGL。

### 任务拆分

- [ ] 1. 抽象渲染器接口（IVideoRenderer）
  - [x] 1.1 定义 IRenderer 接口
  - [x] 1.2 定义支持的渲染类型（Software, D3D11, OpenGL）
  - [x] 1.3 定义初始化和渲染方法

- [ ] 2. 实现 SDL2 软件渲染器（复用现有）
  - [ ] 2.1 优化现有 Display 类
  - [ ] 2.2 添加双缓冲支持

- [ ] 3. 实现 D3D11 渲染器
  - [x] 3.1 创建 D3D11Renderer 类
  - [ ] 3.2 实现 Direct3D 设备初始化
  - [ ] 3.3 实现纹理创建和更新
  - [ ] 3.4 实现视频帧显示

- [ ] 4. 实现 OpenGL 渲染器
  - [x] 4.1 创建 OpenGLRenderer 类
  - [ ] 4.2 实现 OpenGL 上下文初始化
  - 4.3 实现着色器程序
  - [ ] 4.4 实现 YUV 转 RGB 并渲染

- [ ] 5. 渲染器管理器
  - [x] 5.1 实现 RendererFactory
  - [x] 5.2 实现渲染器自动选择逻辑

### 里程碑
- 支持多种渲染后端
- 渲染器可热切换

---

## 模块 04：音频渲染器

### 目标
实现高质量音频输出，支持多种音频设备和效果。

### 任务拆分

- [ ] 1. 扩展 AudioPlayer 类
  - [x] 1.1 添加音频缓冲管理
  - [x] 1.2 实现流式播放
  - [x] 1.3 添加音量控制

- [ ] 2. 实现音频均衡器
  - [x] 2.1 设计 EQ 参数结构
  - [x] 2.2 实现 10 段均衡器
  - [ ] 2.3 添加预设管理

- [ ] 3. 实现音频混音器
  - [x] 3.1 支持多音频流混合
  - [x] 3.2 实现音量平衡

### 里程碑
- 音频播放无中断
- 均衡器实时生效

---

## 模块 05：字幕系统

### 目标
支持内嵌和外置字幕渲染，包括 ASS/SSA、SRT 格式。

### 任务拆分

- [ ] 1. 字幕解析器基类
  - [x] 1.1 定义 ISubtitleParser 接口
  - [x] 1.2 定义 SubtitleItem 结构

- [ ] 2. 实现 SRT 解析器
  - [x] 2.1 实现时间码解析
  - [x] 2.2 实现文本解析

- [ ] 3. 实现 ASS/SSA 解析器
  - [ ] 3.1 实现 ASS 脚本结构解析
  - [ ] 3.2 实现样式解析
  - [ ] 3.3 实现事件解析

- [ ] 4. 字幕渲染器
  - [ ] 4.1 创建 SubtitleRenderer 类
  - [ ] 4.2 实现基于 SDL2 的文本渲染
  - [ ] 4.3 实现 ASS 图形渲染（可选）

### 里程碑
- 支持 SRT、ASS 格式
- 字幕与视频同步

---

## 模块 06：播放列表

### 目标
管理媒体文件列表，支持持久化和各种排序方式。

### 任务拆分

- [ ] 1. 定义播放列表数据结构
  - [x] 1.1 定义 PlaylistItem 类
  - [x] 1.2 实现媒体信息（时长、分辨率、编码）

- [ ] 2. 实现播放列表管理器
  - [x] 2.1 创建 PlaylistManager 类
  - [x] 2.2 实现 add/remove/clear 方法
  - [x] 2.3 实现多种排序方式（名称、时间、时长）

- [ ] 3. 持久化支持
  - [x] 3.1 支持 M3U8 格式导入/导出
  - [ ] 3.2 实现播放历史记录

### 里程碑
- 播放列表可保存和加载

---

## 模块 07：解码器管理

### 目标
管理软解和硬解解码器，支持自动选择最优解码器。

### 任务拆分

- [ ] 1. 解码器接口抽象
  - [x] 1.1 定义 IDecoder 接口
  - [x] 1.2 定义 DecoderCapability 结构

- [ ] 2. 解码器工厂
  - [x] 2.1 创建 DecoderFactory 类
  - [x] 2.2 实现软解/硬解自动选择

- [ ] 3. 硬件加速支持
  - [x] 3.1 检测系统硬件能力
  - [ ] 3.2 实现 CUDA 解码器封装
  - [ ] 3.3 实现 D3D11VA 解码器封装

### 里程碑
- 自动选择最优解码路径

---

## 模块 08：文件格式支持

### 目标
支持主流媒体容器格式的解析。

### 任务拆分

- [ ] 1. 扩展 FormatContext 管理
  - [x] 1.1 支持更多容器格式
  - [ ] 1.2 实现流信息探测优化

- [ ] 2. 实现特殊格式支持
  - [x] 2.1 添加 MKV 特性支持（章节、轨道）
  - [x] 2.2 添加 MP4 特性支持（moov 预加载）

### 里程碑
- 支持 mkv、mp4、avi、mov、webm 等格式

---

## 模块 09：流媒体支持

### 目标
支持网络流媒体播放，包括 HLS 和 DASH。

### 任务拆分

- [ ] 1. HTTP 流下载器
  - [x] 1.1 实现 HTTP 分片下载
  - [ ] 1.2 实现下载缓冲管理

- [ ] 2. HLS 支持
  - [x] 2.1 实现 M3U8 解析器
  - [ ] 2.2 实现 TS 分片拼接

- [ ] 3. DASH 支持
  - [x] 3.1 实现 MPD 解析器
  - [ ] 3.2 实现自适应码率切换

### 里程碑
- 支持在线视频播放

---

## 模块 10-14：UI 和辅助系统

### 任务拆分

- [ ] 1. 设置系统
  - [x] 1.1 定义 SettingsManager 类
  - [x] 1.2 实现 INI/JSON 配置存储
  - [x] 1.3 实现设置项热重载

- [ ] 2. 快捷键系统
  - [x] 2.1 定义 HotkeyManager 类
  - [x] 2.2 实现按键绑定配置

- [ ] 3. 皮肤系统（可选）
  - [x] 3.1 定义皮肤引擎接口
  - [x] 3.2 实现基本主题支持

### 里程碑
- 设置可持久化和热重载

---

# 实施优先级建议

## 第一阶段（立即开始）
1. Enterprise Quill Logging（日志系统）
2. 模块 01：核心播放引擎（多线程架构）

## 第二阶段（基础设施完成后）
3. 模块 03：视频渲染器
4. 模块 04：音频渲染器
5. 模块 07：解码器管理

## 第三阶段（核心功能稳定后）
6. 模块 02：滤镜系统
7. 模块 05：字幕系统
8. 模块 08：文件格式支持

## 第四阶段（功能完善）
9. 模块 09：流媒体支持
10. 模块 06：播放列表
11. 模块 10-14：UI 辅助系统

