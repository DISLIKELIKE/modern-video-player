#include "playlist/playlist_manager.h"

#include <algorithm>
#include <fstream>

namespace vp::playlist {

/// 追加一个播放项到列表尾部。
void PlaylistManager::addItem(PlaylistItem item) {
    items_.push_back(std::move(item));
}

/// 删除指定索引的播放项；越界时返回 false。
bool PlaylistManager::removeItem(size_t index) {
    if (index >= items_.size()) {
        return false;
    }
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

/// 清空全部播放项。
void PlaylistManager::clear() {
    items_.clear();
}

/// 判断播放列表当前是否为空。
bool PlaylistManager::empty() const {
    return items_.empty();
}

/// 返回当前播放项数量。
size_t PlaylistManager::size() const {
    return items_.size();
}

/// 返回播放项数组的只读视图。
const std::vector<PlaylistItem>& PlaylistManager::items() const {
    return items_;
}

/// 按标题字典序排序播放列表。
void PlaylistManager::sortByTitle() {
    std::sort(items_.begin(), items_.end(),
              [](const PlaylistItem& lhs, const PlaylistItem& rhs) { return lhs.title < rhs.title; });
}

/// 按时长升序排序播放列表。
void PlaylistManager::sortByDuration() {
    std::sort(items_.begin(), items_.end(), [](const PlaylistItem& lhs, const PlaylistItem& rhs) {
        return lhs.duration_seconds < rhs.duration_seconds;
    });
}

/// 从 EXTINF/M3U8 文本重建播放列表条目。
bool PlaylistManager::loadM3U8(const std::string& file_path) {
    std::ifstream input(file_path);
    if (!input.good()) {
        return false;
    }

    items_.clear();

    std::string line;
    PlaylistItem current{};
    bool has_pending = false;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.rfind("#EXTINF:", 0) == 0) {
            has_pending = true;
            current = PlaylistItem{};
            const size_t comma = line.find(',');
            if (comma != std::string::npos) {
                try {
                    current.duration_seconds = std::stod(line.substr(8, comma - 8));
                } catch (...) {
                    current.duration_seconds = 0.0;
                }
                current.title = line.substr(comma + 1);
            }
            continue;
        }
        if (line[0] == '#') {
            continue;
        }

        if (!has_pending) {
            current = PlaylistItem{};
        }
        current.uri = line;
        items_.push_back(current);
        has_pending = false;
    }

    return !items_.empty();
}

/// 将当前播放列表导出为标准 `#EXTM3U` 文本。
bool PlaylistManager::saveM3U8(const std::string& file_path) const {
    std::ofstream output(file_path, std::ios::trunc);
    if (!output.good()) {
        return false;
    }

    output << "#EXTM3U\n";
    for (const PlaylistItem& item : items_) {
        output << "#EXTINF:" << item.duration_seconds << "," << item.title << "\n";
        output << item.uri << "\n";
    }
    return true;
}

}  // namespace vp::playlist

