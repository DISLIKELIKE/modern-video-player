#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vp::playlist {

struct PlaylistItem {
    std::string uri;
    std::string title;
    double duration_seconds{0.0};
    int width{0};
    int height{0};
    std::string codec;
};

class PlaylistManager {
public:
    void addItem(PlaylistItem item);
    bool removeItem(size_t index);
    void clear();

    bool empty() const;
    size_t size() const;
    const std::vector<PlaylistItem>& items() const;

    void sortByTitle();
    void sortByDuration();

    bool loadM3U8(const std::string& file_path);
    bool saveM3U8(const std::string& file_path) const;

private:
    std::vector<PlaylistItem> items_;
};

}  // namespace vp::playlist

