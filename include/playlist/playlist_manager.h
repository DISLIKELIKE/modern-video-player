#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vp::playlist {

/// 播放列表中的单个条目描述。
struct PlaylistItem {
    std::string uri;
    std::string title;
    double duration_seconds{0.0};
    int width{0};
    int height{0};
    std::string codec;
};

/// 播放列表管理器；负责条目维护与 M3U8 导入导出。
class PlaylistManager {
public:
    /// 追加一个播放条目。
    void addItem(PlaylistItem item);
    /// 删除指定索引条目；越界时返回 false。
    bool removeItem(size_t index);
    /// 清空所有条目。
    void clear();

    /// 返回列表是否为空。
    bool empty() const;
    /// 返回条目数。
    size_t size() const;
    /// 返回条目只读视图。
    const std::vector<PlaylistItem>& items() const;

    /// 按标题升序排序。
    void sortByTitle();
    /// 按时长升序排序。
    void sortByDuration();

    /// 从本地 M3U8/EXTM3U 文本加载条目。
    bool loadM3U8(const std::string& file_path);
    /// 将当前条目保存为 M3U8/EXTM3U 文本。
    bool saveM3U8(const std::string& file_path) const;

private:
    std::vector<PlaylistItem> items_;
};

}  // namespace vp::playlist