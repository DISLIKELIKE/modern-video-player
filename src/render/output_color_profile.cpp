#include "render/output_color_profile.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace vp::render {

namespace {

constexpr int kDefaultIccLutSize = 17;

struct IccTagView {
    const unsigned char* data{nullptr};
    size_t size{0};
};

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct Matrix3 {
    double m[3][3]{};
};

struct ToneCurve {
    enum class Kind {
        Identity,
        Gamma,
        Table,
        Parametric,
    };

    Kind kind{Kind::Identity};
    double gamma{1.0};
    int function_type{0};
    std::array<double, 7> params{};
    std::vector<double> table;
};

std::string trimAscii(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(start, end - start);
}

bool tryReadFile(const std::string& path, std::vector<unsigned char>& out, std::string& error) {
    out.clear();
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "failed to open file";
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size <= 0) {
        error = "file is empty";
        return false;
    }
    file.seekg(0, std::ios::beg);

    out.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(out.data()), size)) {
        out.clear();
        error = "failed to read file";
        return false;
    }
    error.clear();
    return true;
}

bool ensureRange(size_t offset, size_t size, size_t total) {
    return offset <= total && size <= total - offset;
}

uint16_t readBe16(const unsigned char* data) {
    return static_cast<uint16_t>((static_cast<uint32_t>(data[0]) << 8) |
                                 static_cast<uint32_t>(data[1]));
}

uint32_t readBe32(const unsigned char* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

int32_t readBeS15Fixed16(const unsigned char* data) {
    return static_cast<int32_t>(readBe32(data));
}

double s15Fixed16ToDouble(int32_t value) {
    return static_cast<double>(value) / 65536.0;
}

double u8Fixed8ToDouble(uint16_t value) {
    return static_cast<double>(value) / 256.0;
}

uint32_t fourCc(const char a, const char b, const char c, const char d) {
    return (static_cast<uint32_t>(static_cast<unsigned char>(a)) << 24) |
           (static_cast<uint32_t>(static_cast<unsigned char>(b)) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 8) |
           static_cast<uint32_t>(static_cast<unsigned char>(d));
}

std::string signatureToText(uint32_t signature) {
    std::string text(4, ' ');
    text[0] = static_cast<char>((signature >> 24) & 0xFF);
    text[1] = static_cast<char>((signature >> 16) & 0xFF);
    text[2] = static_cast<char>((signature >> 8) & 0xFF);
    text[3] = static_cast<char>(signature & 0xFF);
    return text;
}

std::string signatureToName(uint32_t signature) {
    if (signature == fourCc('m', 'n', 't', 'r')) {
        return "mntr";
    }
    if (signature == fourCc('R', 'G', 'B', ' ')) {
        return "RGB";
    }
    if (signature == fourCc('X', 'Y', 'Z', ' ')) {
        return "XYZ";
    }
    return trimAscii(signatureToText(signature));
}

bool findTag(const std::vector<unsigned char>& file_data, uint32_t signature, IccTagView& out) {
    out = {};
    if (file_data.size() < 132) {
        return false;
    }
    const uint32_t tag_count = readBe32(file_data.data() + 128);
    const size_t table_offset = 132;
    if (!ensureRange(table_offset, static_cast<size_t>(tag_count) * 12u, file_data.size())) {
        return false;
    }

    for (uint32_t i = 0; i < tag_count; ++i) {
        const size_t entry_offset = table_offset + static_cast<size_t>(i) * 12u;
        const uint32_t entry_signature = readBe32(file_data.data() + entry_offset);
        const uint32_t entry_data_offset = readBe32(file_data.data() + entry_offset + 4u);
        const uint32_t entry_size = readBe32(file_data.data() + entry_offset + 8u);
        if (entry_signature != signature) {
            continue;
        }
        if (!ensureRange(entry_data_offset, entry_size, file_data.size())) {
            return false;
        }
        out.data = file_data.data() + entry_data_offset;
        out.size = entry_size;
        return true;
    }
    return false;
}

bool parseXyzTag(const IccTagView& view, Vec3& out) {
    if (!view.data || view.size < 20) {
        return false;
    }
    if (readBe32(view.data) != fourCc('X', 'Y', 'Z', ' ')) {
        return false;
    }
    out.x = s15Fixed16ToDouble(readBeS15Fixed16(view.data + 8));
    out.y = s15Fixed16ToDouble(readBeS15Fixed16(view.data + 12));
    out.z = s15Fixed16ToDouble(readBeS15Fixed16(view.data + 16));
    return true;
}

std::string utf16BeToUtf8(const unsigned char* data, size_t bytes) {
    if (!data || bytes < 2) {
        return {};
    }

    std::string out;
    out.reserve(bytes / 2u);
    for (size_t i = 0; i + 1 < bytes; i += 2) {
        const uint16_t code_unit = readBe16(data + i);
        if (code_unit == 0) {
            break;
        }
        if (code_unit < 0x80) {
            out.push_back(static_cast<char>(code_unit));
        } else if (code_unit < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code_unit >> 6)));
            out.push_back(static_cast<char>(0x80 | (code_unit & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (code_unit >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code_unit >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code_unit & 0x3F)));
        }
    }
    return trimAscii(out);
}

bool parseDescriptionTag(const IccTagView& view, std::string& out) {
    out = "unknown";
    if (!view.data || view.size < 12) {
        return false;
    }

    const uint32_t type = readBe32(view.data);
    if (type == fourCc('d', 'e', 's', 'c')) {
        if (view.size < 16) {
            return false;
        }
        const uint32_t length = readBe32(view.data + 8);
        if (length == 0) {
            return false;
        }
        const size_t payload_offset = 12;
        const size_t payload_size = static_cast<size_t>(length > 0 ? length - 1u : 0u);
        if (!ensureRange(payload_offset, payload_size, view.size)) {
            return false;
        }
        out.assign(reinterpret_cast<const char*>(view.data + payload_offset), payload_size);
        out = trimAscii(out);
        return !out.empty();
    }

    if (type == fourCc('m', 'l', 'u', 'c')) {
        if (view.size < 16) {
            return false;
        }
        const uint32_t record_count = readBe32(view.data + 8);
        const uint32_t record_size = readBe32(view.data + 12);
        if (record_count == 0 || record_size < 12) {
            return false;
        }
        const size_t record_offset = 16;
        if (!ensureRange(record_offset, static_cast<size_t>(record_count) * record_size, view.size)) {
            return false;
        }
        const unsigned char* record = view.data + record_offset;
        const uint32_t text_length = readBe32(record + 4);
        const uint32_t text_offset = readBe32(record + 8);
        if (!ensureRange(text_offset, text_length, view.size)) {
            return false;
        }
        out = utf16BeToUtf8(view.data + text_offset, text_length);
        return !out.empty();
    }

    return false;
}

bool parseToneCurve(const IccTagView& view, ToneCurve& out) {
    out = ToneCurve{};
    if (!view.data || view.size < 12) {
        return false;
    }

    const uint32_t type = readBe32(view.data);
    if (type == fourCc('c', 'u', 'r', 'v')) {
        const uint32_t count = readBe32(view.data + 8);
        if (count == 0) {
            out.kind = ToneCurve::Kind::Identity;
            return true;
        }
        if (count == 1) {
            if (view.size < 14) {
                return false;
            }
            out.kind = ToneCurve::Kind::Gamma;
            out.gamma = std::max(0.0001, u8Fixed8ToDouble(readBe16(view.data + 12)));
            return true;
        }
        if (!ensureRange(12, static_cast<size_t>(count) * 2u, view.size)) {
            return false;
        }
        out.kind = ToneCurve::Kind::Table;
        out.table.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            out.table[i] = static_cast<double>(readBe16(view.data + 12 + static_cast<size_t>(i) * 2u)) / 65535.0;
        }
        return true;
    }

    if (type == fourCc('p', 'a', 'r', 'a')) {
        if (view.size < 16) {
            return false;
        }
        out.kind = ToneCurve::Kind::Parametric;
        out.function_type = static_cast<int>(readBe16(view.data + 8));
        const int parameter_count =
            out.function_type == 0 ? 1 :
            out.function_type == 1 ? 3 :
            out.function_type == 2 ? 4 :
            out.function_type == 3 ? 5 :
            out.function_type == 4 ? 7 : 0;
        if (parameter_count == 0 || !ensureRange(12, static_cast<size_t>(parameter_count) * 4u, view.size)) {
            return false;
        }
        for (int i = 0; i < parameter_count; ++i) {
            out.params[static_cast<size_t>(i)] = s15Fixed16ToDouble(readBeS15Fixed16(view.data + 12 + static_cast<size_t>(i) * 4u));
        }
        return true;
    }

    return false;
}

double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

double applyToneCurveForward(const ToneCurve& curve, double value) {
    const double x = clamp01(value);
    switch (curve.kind) {
    case ToneCurve::Kind::Identity:
        return x;
    case ToneCurve::Kind::Gamma:
        return std::pow(x, curve.gamma);
    case ToneCurve::Kind::Table: {
        if (curve.table.empty()) {
            return x;
        }
        if (curve.table.size() == 1) {
            return clamp01(curve.table.front());
        }
        const double position = x * static_cast<double>(curve.table.size() - 1u);
        const size_t index = static_cast<size_t>(std::min<double>(std::floor(position), static_cast<double>(curve.table.size() - 2u)));
        const double t = position - static_cast<double>(index);
        return clamp01(curve.table[index] * (1.0 - t) + curve.table[index + 1u] * t);
    }
    case ToneCurve::Kind::Parametric: {
        const double g = curve.params[0];
        const double a = curve.params[1];
        const double b = curve.params[2];
        const double c = curve.params[3];
        const double d = curve.params[4];
        const double e = curve.params[5];
        const double f = curve.params[6];
        switch (curve.function_type) {
        case 0:
            return clamp01(std::pow(x, g));
        case 1: {
            const double threshold = a != 0.0 ? (-b / a) : 0.0;
            return clamp01(x >= threshold ? std::pow(a * x + b, g) : 0.0);
        }
        case 2: {
            const double threshold = a != 0.0 ? (-b / a) : 0.0;
            return clamp01(x >= threshold ? std::pow(a * x + b, g) + c : c);
        }
        case 3:
            return clamp01(x >= d ? std::pow(a * x + b, g) : c * x);
        case 4:
            return clamp01(x >= d ? std::pow(a * x + b, g) + e : c * x + f);
        default:
            return x;
        }
    }
    }
    return x;
}

double applyToneCurveInverse(const ToneCurve& curve, double value) {
    const double target = clamp01(value);
    double low = 0.0;
    double high = 1.0;
    const double low_value = applyToneCurveForward(curve, low);
    const double high_value = applyToneCurveForward(curve, high);
    if (target <= low_value) {
        return low;
    }
    if (target >= high_value) {
        return high;
    }

    for (int i = 0; i < 28; ++i) {
        const double mid = 0.5 * (low + high);
        const double mid_value = applyToneCurveForward(curve, mid);
        if (mid_value < target) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return clamp01(0.5 * (low + high));
}

Vec3 multiply(const Matrix3& matrix, const Vec3& value) {
    Vec3 out;
    out.x = matrix.m[0][0] * value.x + matrix.m[0][1] * value.y + matrix.m[0][2] * value.z;
    out.y = matrix.m[1][0] * value.x + matrix.m[1][1] * value.y + matrix.m[1][2] * value.z;
    out.z = matrix.m[2][0] * value.x + matrix.m[2][1] * value.y + matrix.m[2][2] * value.z;
    return out;
}

bool invertMatrix(const Matrix3& in, Matrix3& out) {
    const double a = in.m[0][0];
    const double b = in.m[0][1];
    const double c = in.m[0][2];
    const double d = in.m[1][0];
    const double e = in.m[1][1];
    const double f = in.m[1][2];
    const double g = in.m[2][0];
    const double h = in.m[2][1];
    const double i = in.m[2][2];

    const double determinant = a * (e * i - f * h) -
                               b * (d * i - f * g) +
                               c * (d * h - e * g);
    if (std::abs(determinant) < 1e-12) {
        return false;
    }

    const double inv_det = 1.0 / determinant;
    out.m[0][0] = (e * i - f * h) * inv_det;
    out.m[0][1] = (c * h - b * i) * inv_det;
    out.m[0][2] = (b * f - c * e) * inv_det;
    out.m[1][0] = (f * g - d * i) * inv_det;
    out.m[1][1] = (a * i - c * g) * inv_det;
    out.m[1][2] = (c * d - a * f) * inv_det;
    out.m[2][0] = (d * h - e * g) * inv_det;
    out.m[2][1] = (b * g - a * h) * inv_det;
    out.m[2][2] = (a * e - b * d) * inv_det;
    return true;
}

double decodeSrgb(double encoded) {
    const double x = clamp01(encoded);
    if (x <= 0.04045) {
        return x / 12.92;
    }
    return std::pow((x + 0.055) / 1.055, 2.4);
}

constexpr Matrix3 kSrgbToXyzD65{{
    {0.4124564, 0.3575761, 0.1804375},
    {0.2126729, 0.7151522, 0.0721750},
    {0.0193339, 0.1191920, 0.9503041},
}};

constexpr Matrix3 kBradfordD65ToD50{{
    {1.0479298, 0.0229468, -0.0501922},
    {0.0296278, 0.9904345, -0.0170738},
    {-0.0092430, 0.0150552, 0.7518743},
}};

std::vector<std::string> splitAsciiWhitespace(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool tryParseFloatToken(const std::string& token, float& out) {
    if (token.empty()) {
        return false;
    }
    char* end = nullptr;
    const float value = std::strtof(token.c_str(), &end);
    if (end == token.c_str() || (end && *end != '\0') || !std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

bool tryParseIntToken(const std::string& token, int& out) {
    if (token.empty()) {
        return false;
    }
    char* end = nullptr;
    const long value = std::strtol(token.c_str(), &end, 10);
    if (end == token.c_str() || (end && *end != '\0') ||
        value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

}  // namespace

bool loadCubeLut3D(const std::string& path, OutputLut3DData& out, std::string& error) {
    out = OutputLut3DData{};
    std::ifstream file(path);
    if (!file) {
        error = "failed to open LUT file";
        return false;
    }

    int lut_size = 0;
    float domain_min[3] = {0.0f, 0.0f, 0.0f};
    float domain_max[3] = {1.0f, 1.0f, 1.0f};
    std::vector<float> values;
    std::string line;
    while (std::getline(file, line)) {
        const size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line.erase(comment_pos);
        }
        line = trimAscii(line);
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> tokens = splitAsciiWhitespace(line);
        if (tokens.empty()) {
            continue;
        }

        const std::string directive = tokens[0];
        if (directive == "TITLE") {
            continue;
        }
        if (directive == "LUT_3D_SIZE" || directive == "lut_3d_size") {
            if (tokens.size() < 2 || !tryParseIntToken(tokens[1], lut_size) || lut_size < 2 || lut_size > 128) {
                error = "invalid LUT_3D_SIZE";
                return false;
            }
            continue;
        }
        if (directive == "DOMAIN_MIN" || directive == "DOMAIN_MAX" ||
            directive == "domain_min" || directive == "domain_max") {
            if (tokens.size() < 4) {
                error = "invalid DOMAIN_* line";
                return false;
            }
            float parsed[3] = {0.0f, 0.0f, 0.0f};
            if (!tryParseFloatToken(tokens[1], parsed[0]) ||
                !tryParseFloatToken(tokens[2], parsed[1]) ||
                !tryParseFloatToken(tokens[3], parsed[2])) {
                error = "invalid DOMAIN_* values";
                return false;
            }
            float* target = (directive == "DOMAIN_MIN" || directive == "domain_min") ? domain_min : domain_max;
            target[0] = parsed[0];
            target[1] = parsed[1];
            target[2] = parsed[2];
            continue;
        }

        if (tokens.size() < 3) {
            error = "invalid LUT sample line";
            return false;
        }

        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        if (!tryParseFloatToken(tokens[0], red) ||
            !tryParseFloatToken(tokens[1], green) ||
            !tryParseFloatToken(tokens[2], blue)) {
            error = "invalid LUT sample values";
            return false;
        }

        values.push_back(red);
        values.push_back(green);
        values.push_back(blue);
    }

    if (lut_size < 2) {
        error = "LUT_3D_SIZE missing";
        return false;
    }

    const size_t expected_sample_count =
        static_cast<size_t>(lut_size) * static_cast<size_t>(lut_size) * static_cast<size_t>(lut_size);
    if (values.size() / 3u != expected_sample_count) {
        error = "LUT sample count does not match LUT_3D_SIZE";
        return false;
    }

    out.size = lut_size;
    out.rgb8.resize(expected_sample_count * 3u, 0u);
    for (size_t sample_index = 0; sample_index < expected_sample_count; ++sample_index) {
        for (int channel = 0; channel < 3; ++channel) {
            const float min_value = domain_min[channel];
            const float max_value = domain_max[channel];
            const float denom = std::max(1e-6f, max_value - min_value);
            const float normalized = std::clamp((values[sample_index * 3u + static_cast<size_t>(channel)] - min_value) / denom,
                                                0.0f,
                                                1.0f);
            out.rgb8[sample_index * 3u + static_cast<size_t>(channel)] =
                static_cast<unsigned char>(normalized * 255.0f + 0.5f);
        }
    }
    error.clear();
    return true;
}

bool generateIccProfileLut(const std::string& path,
                           int lut_size,
                           OutputLut3DData& out,
                           IccProfileSummary& summary,
                           std::string& error) {
    out = OutputLut3DData{};
    summary = IccProfileSummary{};
    std::vector<unsigned char> file_data;
    if (!tryReadFile(path, file_data, error)) {
        return false;
    }
    if (file_data.size() < 132) {
        error = "ICC profile is too small";
        return false;
    }

    const uint32_t device_class = readBe32(file_data.data() + 12);
    const uint32_t color_space = readBe32(file_data.data() + 16);
    const uint32_t connection_space = readBe32(file_data.data() + 20);
    summary.device_class = signatureToName(device_class);
    summary.color_space = signatureToName(color_space);
    summary.connection_space = signatureToName(connection_space);

    IccTagView description_view;
    if (findTag(file_data, fourCc('d', 'e', 's', 'c'), description_view)) {
        std::string description;
        if (parseDescriptionTag(description_view, description) && !description.empty()) {
            summary.description = description;
        }
    }

    if (color_space != fourCc('R', 'G', 'B', ' ') || connection_space != fourCc('X', 'Y', 'Z', ' ')) {
        error = "unsupported ICC profile color space (expected RGB -> XYZ)";
        return false;
    }

    IccTagView red_xyz_view;
    IccTagView green_xyz_view;
    IccTagView blue_xyz_view;
    IccTagView red_trc_view;
    IccTagView green_trc_view;
    IccTagView blue_trc_view;
    if (!findTag(file_data, fourCc('r', 'X', 'Y', 'Z'), red_xyz_view) ||
        !findTag(file_data, fourCc('g', 'X', 'Y', 'Z'), green_xyz_view) ||
        !findTag(file_data, fourCc('b', 'X', 'Y', 'Z'), blue_xyz_view) ||
        !findTag(file_data, fourCc('r', 'T', 'R', 'C'), red_trc_view) ||
        !findTag(file_data, fourCc('g', 'T', 'R', 'C'), green_trc_view) ||
        !findTag(file_data, fourCc('b', 'T', 'R', 'C'), blue_trc_view)) {
        error = "ICC profile is missing RGB matrix/TRC tags";
        return false;
    }

    Vec3 red_xyz;
    Vec3 green_xyz;
    Vec3 blue_xyz;
    ToneCurve red_trc;
    ToneCurve green_trc;
    ToneCurve blue_trc;
    if (!parseXyzTag(red_xyz_view, red_xyz) ||
        !parseXyzTag(green_xyz_view, green_xyz) ||
        !parseXyzTag(blue_xyz_view, blue_xyz) ||
        !parseToneCurve(red_trc_view, red_trc) ||
        !parseToneCurve(green_trc_view, green_trc) ||
        !parseToneCurve(blue_trc_view, blue_trc)) {
        error = "failed to parse ICC matrix/TRC data";
        return false;
    }

    Matrix3 device_to_xyz_d50{};
    device_to_xyz_d50.m[0][0] = red_xyz.x;
    device_to_xyz_d50.m[0][1] = green_xyz.x;
    device_to_xyz_d50.m[0][2] = blue_xyz.x;
    device_to_xyz_d50.m[1][0] = red_xyz.y;
    device_to_xyz_d50.m[1][1] = green_xyz.y;
    device_to_xyz_d50.m[1][2] = blue_xyz.y;
    device_to_xyz_d50.m[2][0] = red_xyz.z;
    device_to_xyz_d50.m[2][1] = green_xyz.z;
    device_to_xyz_d50.m[2][2] = blue_xyz.z;

    Matrix3 xyz_d50_to_device{};
    if (!invertMatrix(device_to_xyz_d50, xyz_d50_to_device)) {
        error = "ICC RGB matrix is singular";
        return false;
    }

    const int effective_lut_size = lut_size >= 2 ? lut_size : kDefaultIccLutSize;
    const size_t sample_count =
        static_cast<size_t>(effective_lut_size) * static_cast<size_t>(effective_lut_size) * static_cast<size_t>(effective_lut_size);
    out.size = effective_lut_size;
    out.rgb8.resize(sample_count * 3u, 0u);

    size_t sample_index = 0;
    for (int blue_index = 0; blue_index < effective_lut_size; ++blue_index) {
        const double blue_encoded = static_cast<double>(blue_index) / static_cast<double>(effective_lut_size - 1);
        for (int green_index = 0; green_index < effective_lut_size; ++green_index) {
            const double green_encoded = static_cast<double>(green_index) / static_cast<double>(effective_lut_size - 1);
            for (int red_index = 0; red_index < effective_lut_size; ++red_index) {
                const double red_encoded = static_cast<double>(red_index) / static_cast<double>(effective_lut_size - 1);

                const Vec3 source_linear{
                    decodeSrgb(red_encoded),
                    decodeSrgb(green_encoded),
                    decodeSrgb(blue_encoded),
                };
                const Vec3 source_xyz_d65 = multiply(kSrgbToXyzD65, source_linear);
                const Vec3 source_xyz_d50 = multiply(kBradfordD65ToD50, source_xyz_d65);
                Vec3 device_linear = multiply(xyz_d50_to_device, source_xyz_d50);
                device_linear.x = clamp01(device_linear.x);
                device_linear.y = clamp01(device_linear.y);
                device_linear.z = clamp01(device_linear.z);

                const double device_red = applyToneCurveInverse(red_trc, device_linear.x);
                const double device_green = applyToneCurveInverse(green_trc, device_linear.y);
                const double device_blue = applyToneCurveInverse(blue_trc, device_linear.z);

                out.rgb8[sample_index * 3u + 0u] = static_cast<unsigned char>(clamp01(device_red) * 255.0 + 0.5);
                out.rgb8[sample_index * 3u + 1u] = static_cast<unsigned char>(clamp01(device_green) * 255.0 + 0.5);
                out.rgb8[sample_index * 3u + 2u] = static_cast<unsigned char>(clamp01(device_blue) * 255.0 + 0.5);
                ++sample_index;
            }
        }
    }

    summary.valid = true;
    summary.matrix_shaper_rgb = true;
    error.clear();
    return true;
}

}  // namespace vp::render
