# 需求实施计划: 修复崩溃并重构为企业级多线程播放器架构

## 第一阶段: 修复当前崩溃问题

- [ ] 1. 修复 PacketReaderThread 中的双重释放和日志问题
  - [ ] 1.1 修复 readLoop 中的双重释放 bug
    - 移除 PacketRef 被移动后的 av_packet_free 调用
    - 在创建 PacketRef 之前保存 stream_index
    
  - [ ] 1.2 修复日志格式问题
    - 将 `{}` 占位符改为 `<<` 操作符格式

- [ ] 2. 检查点 - 编译验证修复
  - 编译项目确保无错误

---

## 第二阶段: 企业级多线程架构重构

- [ ] 3. 设计并实现核心组件接口
  - [ ] 3.1 创建 Demuxer (解封装器) 类
    - 封装 AVFormatContext 的读取操作
    - 提供统一的 packet 读取接口
    - 支持 seek 操作
    
  - [ ] 3.2 创建 DecoderWorker (解码工作线程) 类
    - 封装单个流的解码逻辑
    - 从 PacketQueue 获取 packet，解码后放入 FrameQueue
    - 支持暂停/恢复/flush
    
  - [ ] 3.3 创建 Clock (时钟同步) 类
    - 管理主时钟
    - 计算音视频同步延迟
    - 支持多种同步模式

- [ ] 4. 重构 PacketQueue 和 FrameQueue
  - [ ] 4.1 优化 PacketQueue 实现
    - 确保线程安全
    - 支持 EOF 信号传递
    
  - [ ] 4.2 优化 FrameQueue 实现
    - 确保线程安全
    - 支持非阻塞和阻塞操作

- [ ] 5. 重构 VideoPlayer 整合新架构
  - [ ] 5.1 简化 VideoPlayer 职责
    - 只负责组件协调和状态管理
    - 移除直接的 FFmpeg 调用
    
  - [ ] 5.2 实现新的播放流程
    - open(): 初始化 Demuxer，获取流信息
    - play(): 启动 Demuxer 线程、Decoder 线程、Renderer 线程
    - stop(): 按顺序停止所有线程

- [ ] 6. 更新文档
  - [ ] 6.1 更新 ARCHITECTURE.md
    - 描述新的架构设计
    - 更新架构图
    
  - [ ] 6.2 更新 CHANGELOG.md
    - 记录本次重构的变更
