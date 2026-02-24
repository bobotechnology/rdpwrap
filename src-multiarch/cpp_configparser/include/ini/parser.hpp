#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ini/error.hpp"

namespace ini {

static constexpr int kDefaultInterpolationDepth = 10;
static constexpr const char* kDefaultSectionName = "DEFAULT";
static constexpr const char* kUnnamedSectionName = "<UNNAMED_SECTION>";

enum class InterpolationMode {
    None,
    Basic,
    Extended,
};

struct ParseOptions {
    bool allow_no_value = false;
    bool strict = true;
    bool empty_lines_in_values = true;
    bool allow_unnamed_section = false;

    std::string default_section = kDefaultSectionName;

    std::vector<std::string> delimiters = {"=", ":"};
    std::vector<std::string> comment_prefixes = {"#", ";"};
    std::vector<std::string> inline_comment_prefixes = {};

    InterpolationMode interpolation = InterpolationMode::Basic;
    int max_interpolation_depth = kDefaultInterpolationDepth;
};

using OptionValue = std::optional<std::string>;
using OptionEntry = std::pair<std::string, OptionValue>;
using SectionItems = std::vector<OptionEntry>;

class SectionView {
public:
    SectionView() = default;
    explicit SectionView(const SectionItems* items) : items_(items) {}

    bool has_option(std::string_view option) const;
    const OptionValue& at(std::string_view option) const;
    const SectionItems& items() const;

private:
    const SectionItems* items_ = nullptr;
};

class Parser {
public:
    explicit Parser(ParseOptions options = {});

    void clear();

    void add_section(std::string section);

    bool has_section(std::string_view section) const;
    bool has_option(std::string_view section, std::string_view option) const;

    std::vector<std::string> sections() const;
    std::vector<std::string> options(std::string_view section) const;

    void read_string(std::string_view text, std::string_view source = "<string>");
    void read_file(std::string_view path);

    std::string get(std::string_view section, std::string_view option) const;
    OptionValue get_raw(std::string_view section, std::string_view option) const;
    int get_int(std::string_view section, std::string_view option) const;
    double get_float(std::string_view section, std::string_view option) const;
    bool get_bool(std::string_view section, std::string_view option) const;

    SectionItems items(std::string_view section, bool raw = false) const;

    void set(std::string_view section, std::string option, OptionValue value);

    bool remove_option(std::string_view section, std::string_view option);
    bool remove_section(std::string_view section);

    std::string write_to_string(bool space_around_delimiters = true) const;

    const ParseOptions& parse_options() const noexcept { return options_; }

private:
    std::string option_xform(std::string_view option) const;
    std::string strip_comment(std::string_view line) const;
    void parse_line(
        const std::string& original_line,
        const std::string& cleaned_line,
        const std::string& source,
        int line_no,
        std::string& current_section,
        std::string& current_option,
        std::vector<std::string>& multiline_accum,
        bool& in_multiline,
        std::vector<ParsingError>& errors);
    void finish_multiline(
        const std::string& section,
        const std::string& option,
        std::vector<std::string>& multiline_accum);
    void join_multiline_values();

    std::string interpolate(
        std::string_view section,
        std::string_view option,
        std::string_view value,
        int depth) const;

    std::string interpolate_basic(
        std::string_view section,
        std::string_view option,
        std::string_view value,
        int depth) const;

    std::string interpolate_extended(
        std::string_view section,
        std::string_view option,
        std::string_view value,
        int depth) const;

    const SectionItems* find_section_items(std::string_view section) const;
    SectionItems* find_section_items_mut(std::string_view section);

private:
    ParseOptions options_;

    SectionItems defaults_;
    std::vector<std::pair<std::string, SectionItems>> sections_;

    std::unordered_map<std::string, std::size_t> section_index_;
};

}  // namespace ini
