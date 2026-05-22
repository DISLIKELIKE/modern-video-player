#include "subtitle/subtitle_font_registry.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(_WIN32) && !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if defined(_WIN32)
#include <windows.h>
#include <dwrite.h>
#include <dwrite_3.h>
#include <wrl/client.h>
#endif
#if defined(__linux__)
#include <fontconfig/fontconfig.h>
#endif

extern "C" {
#include <libavformat/avformat.h>
}

namespace vp::subtitle {

namespace {

using Path = std::filesystem::path;

struct MediaAttachmentFontSession {
    SubtitleFontRegistrationSummary summary;
    std::vector<Path> extracted_files;
    std::vector<Path> registered_files;
    Path cache_dir;
};

struct SubtitleFontRegistryState {
    std::mutex mutex;
    std::unordered_map<std::string, SubtitleFontRegistrationSummary> per_source;
    std::unordered_map<std::string, MediaAttachmentFontSession> media_sessions;
    std::unordered_map<std::string, Path> registered_font_files;
};

SubtitleFontRegistryState& registryState() {
    static SubtitleFontRegistryState state;
    return state;
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string normalizePathKey(const std::string& path_value) {
    if (path_value.empty()) {
        return std::string();
    }

    std::error_code ec;
    const Path raw = Path(path_value).lexically_normal();
    const Path normalized = std::filesystem::weakly_canonical(raw, ec);
    const Path effective = ec ? raw : normalized;
    return toLowerAscii(effective.u8string());
}

std::string normalizeFileKey(const Path& path_value) {
    std::error_code ec;
    const Path normalized = std::filesystem::weakly_canonical(path_value, ec);
    const Path effective = ec ? path_value.lexically_normal() : normalized;
    return toLowerAscii(effective.u8string());
}

bool isFontExtension(const Path& path) {
    const std::string extension = toLowerAscii(path.extension().string());
    return extension == ".ttf" || extension == ".otf" || extension == ".ttc" || extension == ".otc";
}

void appendUniquePath(std::vector<Path>& paths,
                      std::unordered_set<std::string>& seen,
                      const Path& path) {
    if (path.empty()) {
        return;
    }

    std::error_code ec;
    const Path normalized = std::filesystem::weakly_canonical(path, ec);
    const Path effective = ec ? path.lexically_normal() : normalized;
    const std::string key = toLowerAscii(effective.u8string());
    if (!seen.insert(key).second) {
        return;
    }
    paths.push_back(effective);
}

std::vector<Path> collectCandidateDirectories(const std::string& subtitle_source_path) {
    std::vector<Path> directories;
    std::unordered_set<std::string> seen;

    std::error_code ec;
    const Path current = std::filesystem::current_path(ec);
    if (!ec && !current.empty()) {
        appendUniquePath(directories, seen, current / "fonts");
        appendUniquePath(directories, seen, current / "Fonts");
        appendUniquePath(directories, seen, current / "subtitles" / "fonts");
    }

    if (!subtitle_source_path.empty()) {
        const Path source_path = Path(subtitle_source_path).lexically_normal();
        const Path source_dir = source_path.parent_path();
        if (!source_dir.empty()) {
            appendUniquePath(directories, seen, source_dir);
            appendUniquePath(directories, seen, source_dir / "fonts");
            appendUniquePath(directories, seen, source_dir / "Fonts");
            appendUniquePath(directories, seen, source_dir / "attachments");
            appendUniquePath(directories, seen, source_dir / "attachments" / "fonts");
        }
    }

    return directories;
}

void collectFontFilesFromDirectory(const Path& directory,
                                   std::vector<Path>& font_files,
                                   std::unordered_set<std::string>& seen_files) {
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || ec || !std::filesystem::is_directory(directory, ec) || ec) {
        return;
    }

    const bool recursive = toLowerAscii(directory.filename().u8string()).find("font") != std::string::npos;
    if (recursive) {
        for (std::filesystem::recursive_directory_iterator it(directory, ec), end; !ec && it != end; it.increment(ec)) {
            if (ec || !it->is_regular_file(ec) || ec || !isFontExtension(it->path())) {
                continue;
            }
            const std::string key = toLowerAscii(it->path().lexically_normal().u8string());
            if (seen_files.insert(key).second) {
                font_files.push_back(it->path().lexically_normal());
            }
        }
        return;
    }

    for (std::filesystem::directory_iterator it(directory, ec), end; !ec && it != end; it.increment(ec)) {
        if (ec || !it->is_regular_file(ec) || ec || !isFontExtension(it->path())) {
            continue;
        }
        const std::string key = toLowerAscii(it->path().lexically_normal().u8string());
        if (seen_files.insert(key).second) {
            font_files.push_back(it->path().lexically_normal());
        }
    }
}

#if defined(_WIN32)
bool registerPrivateFontFile(const Path& font_path) {
    return AddFontResourceExW(font_path.c_str(), FR_PRIVATE, nullptr) > 0;
}

bool unregisterPrivateFontFile(const Path& font_path) {
    return RemoveFontResourceExW(font_path.c_str(), FR_PRIVATE, nullptr) != 0;
}
#elif defined(__linux__)
FcConfig* currentFontConfig() {
    FcConfig* config = FcConfigGetCurrent();
    if (!config) {
        if (FcInit() == FcFalse) {
            return nullptr;
        }
        config = FcConfigGetCurrent();
    }
    return config;
}

bool addAppFontFile(FcConfig* config, const Path& font_path) {
    if (!config) {
        return false;
    }
    const std::string utf8_path = font_path.u8string();
    if (utf8_path.empty()) {
        return false;
    }
    return FcConfigAppFontAddFile(config,
                                  reinterpret_cast<const FcChar8*>(utf8_path.c_str())) == FcTrue;
}

bool rebuildLinuxAppFontCollection(const std::vector<Path>& font_files) {
    FcConfig* config = currentFontConfig();
    if (!config) {
        return false;
    }

    FcConfigAppFontClear(config);
    bool add_ok = true;
    for (const Path& font_file : font_files) {
        if (!addAppFontFile(config, font_file)) {
            add_ok = false;
        }
    }

    const bool build_ok = FcConfigBuildFonts(config) == FcTrue;
    return add_ok && build_ok;
}

bool registerPrivateFontFile(const Path& font_path) {
    FcConfig* config = currentFontConfig();
    if (!config) {
        return false;
    }
    if (!addAppFontFile(config, font_path)) {
        return false;
    }
    return FcConfigBuildFonts(config) == FcTrue;
}

bool unregisterPrivateFontFile(const Path&) {
    // Fontconfig has no per-file remove API for app fonts; callers should rebuild
    // the app font collection from the remaining registry set.
    return true;
}
#else
bool registerPrivateFontFile(const Path&) {
    return false;
}

bool unregisterPrivateFontFile(const Path&) {
    return false;
}
#endif

void appendUniqueFamily(std::vector<std::wstring>& families,
                        std::unordered_set<std::string>& seen,
                        const std::wstring& family) {
    if (family.empty()) {
        return;
    }

    std::string key;
    key.reserve(family.size());
    for (wchar_t ch : family) {
        const wchar_t lowered = (ch >= L'A' && ch <= L'Z') ? static_cast<wchar_t>(ch - L'A' + L'a') : ch;
        key.push_back(static_cast<char>(lowered <= 0x7f ? lowered : '?'));
    }
    if (!seen.insert(key).second) {
        return;
    }
    families.push_back(family);
}

std::wstring utf8ToWideBestEffort(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }

#if defined(_WIN32)
    const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (required <= 1) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring result(static_cast<size_t>(required - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), required);
    return result;
#else
    return std::wstring(text.begin(), text.end());
#endif
}

uint64_t fnv1a64(const std::string& value) {
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char ch : value) {
        hash ^= static_cast<uint64_t>(ch);
        hash *= 1099511628211ull;
    }
    return hash;
}

std::string formatHex(uint64_t value) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << value;
    return oss.str();
}

std::string sanitizeFilenameComponent(const std::string& text, const std::string& fallback) {
    std::string sanitized;
    sanitized.reserve(text.size());
    for (unsigned char ch : text) {
        if (std::isalnum(ch) || ch == '.' || ch == '_' || ch == '-') {
            sanitized.push_back(static_cast<char>(ch));
        } else {
            sanitized.push_back('_');
        }
    }

    while (!sanitized.empty() && (sanitized.front() == '.' || sanitized.front() == '_' || sanitized.front() == '-')) {
        sanitized.erase(sanitized.begin());
    }
    while (!sanitized.empty() && (sanitized.back() == '.' || sanitized.back() == '_' || sanitized.back() == '-')) {
        sanitized.pop_back();
    }

    return sanitized.empty() ? fallback : sanitized;
}

std::string stripMimeParameters(const std::string& mime_type) {
    const std::string lowered = toLowerAscii(mime_type);
    const size_t semicolon = lowered.find(';');
    return lowered.substr(0, semicolon);
}

std::string fontExtensionForMimeType(const std::string& mime_type) {
    const std::string normalized = stripMimeParameters(mime_type);
    if (normalized == "application/x-truetype-font" ||
        normalized == "application/x-font-ttf" ||
        normalized == "application/font-ttf" ||
        normalized == "font/ttf" ||
        normalized == "application/x-font-truetype" ||
        normalized == "application/x-truetype") {
        return ".ttf";
    }

    if (normalized == "application/vnd.ms-opentype" ||
        normalized == "application/font-sfnt" ||
        normalized == "application/font-otf" ||
        normalized == "application/x-font-otf" ||
        normalized == "font/otf") {
        return ".otf";
    }

    if (normalized == "font/collection" ||
        normalized == "application/x-font-ttc") {
        return ".ttc";
    }

    if (normalized == "application/x-font-otc") {
        return ".otc";
    }

    return std::string();
}

bool isFontMimeType(const std::string& mime_type) {
    const std::string normalized = stripMimeParameters(mime_type);
    return !fontExtensionForMimeType(normalized).empty() ||
           normalized.rfind("font/", 0) == 0 ||
           normalized == "application/font-sfnt";
}

std::string metadataValue(const AVDictionary* metadata, const char* key) {
    if (!metadata || !key || !*key) {
        return std::string();
    }

    const AVDictionaryEntry* entry = av_dict_get(metadata, key, nullptr, 0);
    return (entry && entry->value) ? std::string(entry->value) : std::string();
}

std::string normalizeAttachmentFilename(const std::string& filename,
                                        const std::string& mime_type,
                                        int stream_index) {
    const std::string fallback_extension = fontExtensionForMimeType(mime_type);
    std::string normalized = sanitizeFilenameComponent(Path(filename).filename().u8string(),
                                                       "attachment-font-" + std::to_string(stream_index));

    if (normalized.find('.') == std::string::npos) {
        normalized += fallback_extension.empty() ? ".ttf" : fallback_extension;
    } else if (!isFontExtension(Path(normalized)) && !fallback_extension.empty()) {
        normalized += fallback_extension;
    }

    if (!isFontExtension(Path(normalized))) {
        normalized += ".ttf";
    }

    return normalized;
}

bool ensureDirectoryReady(const Path& directory) {
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    return !ec;
}

bool writeBinaryFile(const Path& file_path, const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return false;
    }

    std::ofstream stream(file_path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return stream.good();
}

Path mediaAttachmentCacheRoot() {
    std::error_code ec;
    const Path temp_root = std::filesystem::temp_directory_path(ec);
    if (ec || temp_root.empty()) {
        return Path("subtitle-font-cache");
    }
    return temp_root / "modern-video-player" / "subtitle-font-cache";
}

Path mediaAttachmentCacheDirectory(const std::string& media_source_path) {
    const Path source_path(media_source_path);
    const std::string stem = sanitizeFilenameComponent(source_path.stem().u8string(), "media");
    const std::string hash = formatHex(fnv1a64(media_source_path.empty() ? std::string("<empty>") : media_source_path));
    return mediaAttachmentCacheRoot() / (stem + "-" + hash);
}

std::string uniqueAttachmentFilename(const std::string& normalized_name,
                                     int stream_index,
                                     std::unordered_set<std::string>& seen_names) {
    if (seen_names.insert(toLowerAscii(normalized_name)).second) {
        return normalized_name;
    }

    const Path path_value(normalized_name);
    const std::string stem = path_value.stem().u8string();
    const std::string ext = path_value.extension().u8string();
    const std::string uniqued = stem + "-" + std::to_string(stream_index) + ext;
    seen_names.insert(toLowerAscii(uniqued));
    return uniqued;
}

std::vector<Path> collectRegisteredPrivateFontFilesLocked(const SubtitleFontRegistryState& state) {
    std::vector<Path> files;
    files.reserve(state.registered_font_files.size());
    for (const auto& entry : state.registered_font_files) {
        files.push_back(entry.second);
    }
    std::sort(files.begin(), files.end(), [](const Path& lhs, const Path& rhs) {
        return lhs.native() < rhs.native();
    });
    return files;
}

#if defined(__linux__)
void rebuildLinuxAppFontCollectionLocked(const SubtitleFontRegistryState& state) {
    const std::vector<Path> registered_font_files = collectRegisteredPrivateFontFilesLocked(state);
    rebuildLinuxAppFontCollection(registered_font_files);
}
#endif

}  // namespace

SubtitleFontRegistrationSummary ensureSubtitleFontsRegistered(const std::string& subtitle_source_path) {
    SubtitleFontRegistryState& state = registryState();
    const std::string source_key = normalizePathKey(subtitle_source_path);

    std::lock_guard<std::mutex> lock(state.mutex);
    if (const auto it = state.per_source.find(source_key); it != state.per_source.end()) {
        return it->second;
    }

    SubtitleFontRegistrationSummary summary;
    summary.source_registered = !subtitle_source_path.empty();

    const std::vector<Path> directories = collectCandidateDirectories(subtitle_source_path);
    for (const Path& directory : directories) {
        summary.search_paths.push_back(directory.u8string());
    }

    std::vector<Path> font_files;
    std::unordered_set<std::string> seen_files;
    for (const Path& directory : directories) {
        collectFontFilesFromDirectory(directory, font_files, seen_files);
    }
    summary.discovered_file_count = font_files.size();

    for (const Path& font_file : font_files) {
        const std::string file_key = normalizeFileKey(font_file);
        if (state.registered_font_files.find(file_key) != state.registered_font_files.end()) {
            continue;
        }
        if (registerPrivateFontFile(font_file)) {
            state.registered_font_files.emplace(file_key, font_file);
            summary.registered_font_files.push_back(font_file.u8string());
            ++summary.registered_file_count;
        }
    }

    state.per_source.emplace(source_key, summary);
    return summary;
}

SubtitleFontRegistrationSummary ensureMediaAttachmentFontsRegistered(const std::string& media_source_path,
                                                                    const AVFormatContext* format_ctx) {
    SubtitleFontRegistryState& state = registryState();
    const std::string media_key = normalizePathKey(media_source_path);

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (const auto it = state.media_sessions.find(media_key); it != state.media_sessions.end()) {
            return it->second.summary;
        }
    }

    SubtitleFontRegistrationSummary summary;
    summary.source_registered = !media_source_path.empty() && format_ctx != nullptr;

    if (!format_ctx) {
        return summary;
    }

    MediaAttachmentFontSession session;
    session.summary.source_registered = summary.source_registered;

    const Path cache_dir = mediaAttachmentCacheDirectory(media_source_path);
    std::unordered_set<std::string> seen_output_names;
    bool cache_ready = false;

    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        AVStream* stream = format_ctx->streams[i];
        if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_ATTACHMENT) {
            continue;
        }

        const std::string attachment_filename = metadataValue(stream->metadata, "filename");
        const std::string mime_type = metadataValue(stream->metadata, "mimetype");
        if (!isFontExtension(Path(attachment_filename)) && !isFontMimeType(mime_type)) {
            continue;
        }

        ++session.summary.discovered_attachment_stream_count;

        if (!stream->codecpar->extradata || stream->codecpar->extradata_size <= 0) {
            ++session.summary.invalid_attachment_stream_count;
            continue;
        }

        if (!cache_ready) {
            std::error_code ec;
            std::filesystem::remove_all(cache_dir, ec);
            cache_ready = ensureDirectoryReady(cache_dir);
            if (cache_ready) {
                session.cache_dir = cache_dir;
                session.summary.extraction_cache_path = cache_dir.u8string();
            }
        }

        if (!cache_ready) {
            ++session.summary.invalid_attachment_stream_count;
            continue;
        }

        const std::string normalized_name = normalizeAttachmentFilename(attachment_filename, mime_type, static_cast<int>(i));
        const std::string output_name = uniqueAttachmentFilename(normalized_name, static_cast<int>(i), seen_output_names);
        const Path output_path = cache_dir / output_name;

        if (!writeBinaryFile(output_path,
                             stream->codecpar->extradata,
                             static_cast<size_t>(stream->codecpar->extradata_size))) {
            ++session.summary.invalid_attachment_stream_count;
            continue;
        }

        session.extracted_files.push_back(output_path);
        session.summary.extracted_font_files.push_back(output_path.u8string());
        ++session.summary.extracted_file_count;

        const std::string file_key = normalizeFileKey(output_path);
        bool inserted = false;
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            if (state.registered_font_files.find(file_key) == state.registered_font_files.end() &&
                registerPrivateFontFile(output_path)) {
                state.registered_font_files.emplace(file_key, output_path);
                inserted = true;
            }
        }

        if (inserted) {
            session.registered_files.push_back(output_path);
            session.summary.registered_font_files.push_back(output_path.u8string());
            ++session.summary.registered_file_count;
        }
    }

    summary = session.summary;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.media_sessions.emplace(media_key, session);
    }
    return summary;
}

void releaseMediaAttachmentFonts(const std::string& media_source_path) {
    SubtitleFontRegistryState& state = registryState();
    const std::string media_key = normalizePathKey(media_source_path);
    MediaAttachmentFontSession session;
    bool registry_changed = false;

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        const auto it = state.media_sessions.find(media_key);
        if (it == state.media_sessions.end()) {
            return;
        }
        session = std::move(it->second);
        state.media_sessions.erase(it);

        for (const Path& font_path : session.registered_files) {
            unregisterPrivateFontFile(font_path);
            registry_changed = state.registered_font_files.erase(normalizeFileKey(font_path)) > 0 || registry_changed;
        }

#if defined(__linux__)
        if (registry_changed) {
            rebuildLinuxAppFontCollectionLocked(state);
        }
#endif
    }

    std::error_code ec;
    if (!session.cache_dir.empty()) {
        std::filesystem::remove_all(session.cache_dir, ec);
    } else {
        for (const Path& font_path : session.extracted_files) {
            std::filesystem::remove(font_path, ec);
        }
    }
}

std::vector<std::wstring> buildSubtitleFontFallbackFamilies(const std::string& preferred_family_utf8) {
    std::vector<std::wstring> families;
    std::unordered_set<std::string> seen;

    appendUniqueFamily(families, seen, utf8ToWideBestEffort(preferred_family_utf8));
    appendUniqueFamily(families, seen, L"Microsoft YaHei UI");
    appendUniqueFamily(families, seen, L"Segoe UI");
    appendUniqueFamily(families, seen, L"Microsoft JhengHei UI");
    appendUniqueFamily(families, seen, L"Malgun Gothic");
    appendUniqueFamily(families, seen, L"Yu Gothic UI");
    appendUniqueFamily(families, seen, L"Meiryo");
    appendUniqueFamily(families, seen, L"Segoe UI Symbol");
    appendUniqueFamily(families, seen, L"Segoe UI Emoji");
    appendUniqueFamily(families, seen, L"Arial Unicode MS");
    return families;
}

#if defined(_WIN32)
SubtitleDirectWriteCollectionSummary buildDirectWriteSubtitleFontCollection(
    IDWriteFactory* dwrite_factory,
    IDWriteFontCollection** out_collection) {
    SubtitleDirectWriteCollectionSummary summary;
    summary.attempted = true;
    if (out_collection) {
        *out_collection = nullptr;
    }
    if (!dwrite_factory || !out_collection) {
        summary.factory_available = dwrite_factory != nullptr;
        summary.error = "invalid arguments";
        return summary;
    }

    summary.factory_available = true;

    std::vector<Path> registered_font_files;
    {
        SubtitleFontRegistryState& state = registryState();
        std::lock_guard<std::mutex> lock(state.mutex);
        registered_font_files = collectRegisteredPrivateFontFilesLocked(state);
    }
    summary.registered_font_file_count = registered_font_files.size();
    if (registered_font_files.empty()) {
        summary.error = "no registered private subtitle fonts";
        return summary;
    }

    Microsoft::WRL::ComPtr<IDWriteFactory3> factory3;
    if (FAILED(dwrite_factory->QueryInterface(IID_PPV_ARGS(factory3.GetAddressOf()))) || !factory3) {
        summary.error = "IDWriteFactory3 unavailable";
        return summary;
    }
    summary.factory3_available = true;

    Microsoft::WRL::ComPtr<IDWriteFontSetBuilder> font_set_builder;
    if (FAILED(factory3->CreateFontSetBuilder(font_set_builder.GetAddressOf())) || !font_set_builder) {
        summary.error = "CreateFontSetBuilder failed";
        return summary;
    }

    for (const Path& font_path : registered_font_files) {
        Microsoft::WRL::ComPtr<IDWriteFontFaceReference> font_face_reference;
        if (FAILED(factory3->CreateFontFaceReference(font_path.c_str(),
                                                     nullptr,
                                                     0,
                                                     DWRITE_FONT_SIMULATIONS_NONE,
                                                     font_face_reference.GetAddressOf())) ||
            !font_face_reference) {
            continue;
        }
        if (SUCCEEDED(font_set_builder->AddFontFaceReference(font_face_reference.Get()))) {
            ++summary.added_font_file_count;
        }
    }

    if (summary.added_font_file_count == 0) {
        summary.error = "no registered font file could be added to DirectWrite font set";
        return summary;
    }

    Microsoft::WRL::ComPtr<IDWriteFontSet> font_set;
    if (FAILED(font_set_builder->CreateFontSet(font_set.GetAddressOf())) || !font_set) {
        summary.error = "CreateFontSet failed";
        return summary;
    }

    Microsoft::WRL::ComPtr<IDWriteFontCollection1> font_collection1;
    if (FAILED(factory3->CreateFontCollectionFromFontSet(font_set.Get(), font_collection1.GetAddressOf())) ||
        !font_collection1) {
        summary.error = "CreateFontCollectionFromFontSet failed";
        return summary;
    }

    Microsoft::WRL::ComPtr<IDWriteFontCollection> font_collection;
    if (FAILED(font_collection1.As(&font_collection)) || !font_collection) {
        summary.error = "IDWriteFontCollection conversion failed";
        return summary;
    }

    *out_collection = font_collection.Detach();
    summary.collection_created = true;
    summary.error.clear();
    return summary;
}
#endif

}  // namespace vp::subtitle
