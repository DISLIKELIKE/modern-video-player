#pragma once

#include <string>
#include <vector>

struct AVFormatContext;

namespace vp::subtitle {

struct SubtitleFontRegistrationSummary {
    bool source_registered{false};
    size_t discovered_file_count{0};
    size_t registered_file_count{0};
    size_t discovered_attachment_stream_count{0};
    size_t extracted_file_count{0};
    size_t invalid_attachment_stream_count{0};
    std::string extraction_cache_path;
    std::vector<std::string> search_paths;
    std::vector<std::string> extracted_font_files;
    std::vector<std::string> registered_font_files;
};

SubtitleFontRegistrationSummary ensureSubtitleFontsRegistered(const std::string& subtitle_source_path);
SubtitleFontRegistrationSummary ensureMediaAttachmentFontsRegistered(const std::string& media_source_path,
                                                                    const AVFormatContext* format_ctx);
void releaseMediaAttachmentFonts(const std::string& media_source_path);
std::vector<std::wstring> buildSubtitleFontFallbackFamilies(const std::string& preferred_family_utf8);

}  // namespace vp::subtitle
