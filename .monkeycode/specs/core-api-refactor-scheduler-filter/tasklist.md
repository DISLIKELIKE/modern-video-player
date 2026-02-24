# 需求实施计划

- [ ] 1. 设置项目结构和编译配置
   - 创建 `src/core/` 目录用于存放核心模块
   - 创建 `src/filters/` 目录用于存放滤镜插件
   - 创建 `include/core/` 目录用于存放核心头文件
   - 创建 `include/filters/` 目录用于存放滤镜头文件
   - 在 CMakeLists.txt 中添加 `USE_NEW_PLAYER_CORE` 编译选项

- [ ] 2. 实现帧队列基础设施
  - [ ] 2.1 实现 FrameQueue 模板类
    - 创建 `include/core/frame_queue.h` 头文件
    - 实现线程安全的 push/pop 操作（带超时）
    - 实现 flush 清空操作
    - 实现容量配置和填充率查询
    - 参考 Requirement 4: 帧队列系统

  - [ ] 2.2 实现 VideoFrame 和 AudioFrame 数据结构
    - 在 `include/core/frame.h` 中定义 VideoFrame 结构
    - 实现移动语义和资源管理
    - 定义 AudioFrame 结构用于音频样本传输
    - 参考 Data Models: 帧数据结构

  - [ ]* 2.3 为 FrameQueue 编写单元测试
    - 测试并发 push/pop 正确性
    - 测试队列满/空阻塞行为
    - 测试 flush 操作

- [ ] 3. 实现时钟同步器
  - [ ] 3.1 实现 Clock 类
    - 创建 `include/core/clock.h` 和 `src/core/clock.cpp`
    - 实现音频/视频/系统时钟源切换
    - 实现时钟单调性和速度控制
    - 参考 Requirement 5: 音视频同步

  - [ ]* 3.2 为 Clock 编写单元测试
    - 测试时钟单调性
    - 测试时钟源切换正确性

- [ ] 4. 检查点 - 基础设施就绪
  - 确保所有基础类编译通过，如有疑问请询问用户

- [ ] 5. 实现 Core API 层
  - [ ] 5.1 实现 PlayerCore 类基础结构
    - 创建 `include/core/player_core.h` 和 `src/core/player_core.cpp`
    - 定义 PlaybackState, ErrorCode, PlaybackInfo 枚举和结构
    - 实现 open/close/play/pause/stop/seek 接口桩
    - 参考 Requirement 1: Core API Layer

  - [ ] 5.2 实现事件通知系统
    - 在 PlayerCore 中添加回调注册方法
    - 实现 EventDispatcher 辅助类管理回调分发
    - 确保线程安全的回调调用
    - 参考 Requirement 2: Event Notification System

  - [ ] 5.3 实现命令队列
    - 创建 `include/core/command.h` 定义 Command 结构
    - 实现 CommandQueue 用于异步命令处理
    - 参考 Data Models: 命令结构

- [ ] 6. 实现任务调度器
  - [ ] 6.1 实现 Scheduler 类框架
    - 创建 `include/core/scheduler.h` 和 `src/core/scheduler.cpp`
    - 实现线程启动/停止逻辑
    - 实现暂停/恢复机制
    - 参考 Requirement 3: Multi-threaded Scheduler

  - [ ] 6.2 实现视频解码线程
    - 在 Scheduler 中实现 videoDecoderLoop()
    - 集成 FrameQueue 进行帧传递
    - 实现队列背压控制（80% 暂停，50% 恢复）
    - 参考 Requirement 3.2-3.4

  - [ ] 6.3 实现音频解码线程
    - 在 Scheduler 中实现 audioDecoderLoop()
    - 集成音频帧队列
    - 实现队列背压控制
    - 参考 Requirement 3.3-3.5

  - [ ] 6.4 实现渲染线程
    - 在 Scheduler 中实现 renderLoop()
    - 集成 Clock 进行音视频同步
    - 实现帧丢弃逻辑（落后超过 100ms）
    - 参考 Requirement 5.3-5.4

  - [ ]* 6.5 为 Scheduler 编写集成测试
    - 测试线程启动和停止
    - 测试队列填充和消费
    - 测试异常处理

- [ ] 7. 检查点 - 调度器功能验证
  - 确保调度器能正确管理线程，如有疑问请询问用户

- [ ] 8. 实现 Filter 插件框架
  - [ ] 8.1 定义 Filter 接口
    - 创建 `include/filters/video_filter.h` 定义 IVideoFilter 接口
    - 创建 `include/filters/audio_filter.h` 定义 IAudioFilter 接口
    - 定义参数设置/获取接口
    - 参考 Requirement 6: Filter Plugin Framework

  - [ ] 8.2 实现 FilterRegistry
    - 创建 `include/filters/filter_registry.h` 和 `src/filters/filter_registry.cpp`
    - 实现滤镜工厂注册机制
    - 实现滤镜实例创建
    - 参考 Requirement 6.1

  - [ ] 8.3 实现 FilterPipeline
    - 创建 `include/filters/filter_pipeline.h` 和 `src/filters/filter_pipeline.cpp`
    - 实现滤镜链管理
    - 实现异常隔离（单个滤镜失败不影响整体）
    - 参考 Requirement 6.5-6.6

- [ ] 9. 实现内置滤镜
  - [ ] 9.1 实现 BrightnessFilter
    - 创建 `src/filters/brightness_filter.cpp`
    - 实现 YUV 亮度调整算法
    - 参数范围：-100 到 +100
    - 参考 Requirement 7.1

  - [ ] 9.2 实现 ContrastFilter
    - 创建 `src/filters/contrast_filter.cpp`
    - 实现 YUV 对比度调整算法
    - 参数范围：0.5 到 2.0
    - 参考 Requirement 7.2

  - [ ] 9.3 实现 SaturationFilter
    - 创建 `src/filters/saturation_filter.cpp`
    - 实现 YUV 饱和度调整算法
    - 参数范围：0.0 到 2.0
    - 参考 Requirement 7.3

  - [ ] 9.4 注册内置滤镜
    - 创建 `src/filters/builtin_filters.cpp` 实现 registerBuiltinFilters()
    - 在初始化时调用注册函数

- [ ] 10. 检查点 - Filter 系统验证
  - 确保滤镜系统可用，如有疑问请询问用户

- [ ] 11. 集成与迁移
  - [ ] 11.1 实现 VideoPlayer 适配器
    - 修改 `src/video_player.cpp` 使用 Core API
    - 保持原有接口不变
    - 通过 `USE_NEW_PLAYER_CORE` 宏切换实现
    - 参考 Requirement 8: Migration Path

  - [ ] 11.2 更新 CMakeLists.txt
    - 添加新源文件到编译列表
    - 配置条件编译逻辑
    - 链接线程库

  - [ ] 11.3 更新文档
    - 更新 docs/VERSION.md 记录新功能
    - 更新 docs/CHANGELOG.md 添加变更记录
    - 创建 docs/FILTERS.md 说明滤镜使用方法

- [ ] 12. 最终检查点
  - 确保所有测试通过，新旧实现切换正常，如有疑问请询问用户
