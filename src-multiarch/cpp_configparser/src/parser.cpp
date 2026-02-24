#include "ini/parser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace ini {
namespace {

std::string trim_copy(std::string_view sv) {
    std::size_t start = 0;
    while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start])) != 0) {
        ++start;
    }
    std::size_t end = sv.size();
    while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1])) != 0) {
        --end;
    }
    return std::string(sv.substr(start, end - start));
}

bool starts_with(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

std::string join_lines(const std::vector<std::string>& lines) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            oss << '\n';
        }
        oss << lines[i];
    }
    std::string out = oss.str();
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back())) != 0) {
        out.pop_back();
    }
    return out;
}

std::string to_lower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

}  // namespace

bool SectionView::has_option(std::string_view option) const {
    if (items_ == nullptr) {
        return false;
    }
    const auto needle = to_lower(option);
    return std::any_of(items_->begin(), items_->end(), [&](const OptionEntry& e) {
        return e.first == needle;
    });
}

const OptionValue& SectionView::at(std::string_view option) const {
    if (items_ == nullptr) {
        throw Error(ErrorCode::NoSection, "section not found");
    }
    const auto needle = to_lower(option);
    for (const auto& e : *items_) {
        if (e.first == needle) {
            return e.second;
        }
    }
    throw Error(ErrorCode::NoOption, "option not found");
}

const SectionItems& SectionView::items() const {
    if (items_ == nullptr) {
        throw Error(ErrorCode::NoSection, "section not found");
    }
    return *items_;
}

Parser::Parser(ParseOptions options) : options_(std::move(options)) {}

void Parser::clear() {
    defaults_.clear();
    sections_.clear();
    section_index_.clear();
}

std::string Parser::option_xform(std::string_view option) const {
    return to_lower(option);
}

void Parser::add_section(std::string section) {
    if (section == options_.default_section) {
        throw Error(ErrorCode::DuplicateSection, "invalid section name: default section");
    }
    if (section_index_.find(section) != section_index_.end()) {
        throw Error(ErrorCode::DuplicateSection, "duplicate section: " + section);
    }
    const std::size_t idx = sections_.size();
    sections_.push_back({std::move(section), {}});
    section_index_.insert({sections_[idx].first, idx});
}

bool Parser::has_section(std::string_view section) const {
    if (section == options_.default_section) {
        return true;
    }
    return section_index_.find(std::string(section)) != section_index_.end();
}

const SectionItems* Parser::find_section_items(std::string_view section) const {
    if (section == options_.default_section) {
        return &defaults_;
    }
    auto it = section_index_.find(std::string(section));
    if (it == section_index_.end()) {
        return nullptr;
    }
    return &sections_[it->second].second;
}

SectionItems* Parser::find_section_items_mut(std::string_view section) {
    if (section == options_.default_section) {
        return &defaults_;
    }
    auto it = section_index_.find(std::string(section));
    if (it == section_index_.end()) {
        return nullptr;
    }
    return &sections_[it->second].second;
}

bool Parser::has_option(std::string_view section, std::string_view option) const {
    const auto* sec = find_section_items(section);
    if (sec == nullptr) {
        return false;
    }
    const auto needle = option_xform(option);
    for (const auto& e : *sec) {
        if (e.first == needle) {
            return true;
        }
    }
    if (section != options_.default_section) {
        for (const auto& e : defaults_) {
            if (e.first == needle) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> Parser::sections() const {
    std::vector<std::string> out;
    out.reserve(sections_.size());
    for (const auto& sec : sections_) {
        out.push_back(sec.first);
    }
    return out;
}

std::vector<std::string> Parser::options(std::string_view section) const {
    const auto* sec = find_section_items(section);
    if (sec == nullptr) {
        throw Error(ErrorCode::NoSection, "No section: " + std::string(section));
    }
    std::vector<std::string> out;
    for (const auto& e : *sec) {
        out.push_back(e.first);
    }
    if (section != options_.default_section) {
        for (const auto& e : defaults_) {
            if (std::find(out.begin(), out.end(), e.first) == out.end()) {
                out.push_back(e.first);
            }
        }
    }
    return out;
}

std::string Parser::strip_comment(std::string_view line) const {
    auto t = trim_copy(line);
    if (t.empty()) {
        return t;
    }

    for (const auto& prefix : options_.comment_prefixes) {
        if (!prefix.empty() && starts_with(t, prefix)) {
            return "";
        }
    }

    std::string out = std::string(line);
    for (const auto& prefix : options_.inline_comment_prefixes) {
        if (prefix.empty()) {
            continue;
        }
        std::size_t pos = out.find(prefix);
        while (pos != std::string::npos) {
            const bool at_start = (pos == 0);
            const bool has_space_before = !at_start && std::isspace(static_cast<unsigned char>(out[pos - 1])) != 0;
            if (at_start || has_space_before) {
                out.erase(pos);
                break;
            }
            pos = out.find(prefix, pos + 1);
        }
    }

    return trim_copy(out);
}

void Parser::finish_multiline(
    const std::string& section,
    const std::string& option,
    std::vector<std::string>& multiline_accum) {
    if (option.empty()) {
        multiline_accum.clear();
        return;
    }
    auto* sec = find_section_items_mut(section);
    if (sec == nullptr) {
        throw Error(ErrorCode::NoSection, "No section: " + section);
    }
    for (auto& e : *sec) {
        if (e.first == option) {
            e.second = join_lines(multiline_accum);
            multiline_accum.clear();
            return;
        }
    }
    multiline_accum.clear();
}

void Parser::parse_line(
    const std::string& original_line,
    const std::string& cleaned_line,
    const std::string& source,
    int line_no,
    std::string& current_section,
    std::string& current_option,
    std::vector<std::string>& multiline_accum,
    bool& in_multiline,
    std::vector<ParsingError>& errors) {

    const bool indented = !original_line.empty() && std::isspace(static_cast<unsigned char>(original_line[0])) != 0;

    if (cleaned_line.empty()) {
        if (in_multiline && options_.empty_lines_in_values) {
            multiline_accum.push_back("");
        } else {
            in_multiline = false;
        }
        return;
    }

    if (in_multiline && indented && !current_option.empty()) {
        multiline_accum.push_back(cleaned_line);
        return;
    }

    if (in_multiline && !current_option.empty()) {
        finish_multiline(current_section, current_option, multiline_accum);
        in_multiline = false;
    }

    if (cleaned_line.front() == '[' && cleaned_line.back() == ']') {
        current_section = cleaned_line.substr(1, cleaned_line.size() - 2);
        current_option.clear();
        if (current_section.empty()) {
            errors.emplace_back(source, line_no, original_line);
            return;
        }
        if (current_section != options_.default_section && section_index_.find(current_section) == section_index_.end()) {
            const std::size_t idx = sections_.size();
            sections_.push_back({current_section, {}});
            section_index_.insert({current_section, idx});
        } else if (current_section != options_.default_section && options_.strict) {
            throw LocatedError(
                ErrorCode::DuplicateSection,
                "duplicate section: " + current_section,
                source,
                line_no);
        }
        return;
    }

    if (current_section.empty()) {
        if (options_.allow_unnamed_section) {
            current_section = kUnnamedSectionName;
            if (section_index_.find(current_section) == section_index_.end()) {
                const std::size_t idx = sections_.size();
                sections_.push_back({current_section, {}});
                section_index_.insert({current_section, idx});
            }
        } else {
            throw LocatedError(
                ErrorCode::MissingSectionHeader,
                "File contains no section headers",
                source,
                line_no);
        }
    }

    std::size_t found_at = std::string::npos;
    std::string used_delim;
    for (const auto& d : options_.delimiters) {
        auto pos = cleaned_line.find(d);
        if (pos != std::string::npos && (found_at == std::string::npos || pos < found_at)) {
            found_at = pos;
            used_delim = d;
        }
    }

    std::string key;
    OptionValue value;

    if (found_at == std::string::npos) {
        if (!options_.allow_no_value) {
            errors.emplace_back(source, line_no, original_line);
            return;
        }
        key = trim_copy(cleaned_line);
        value = std::nullopt;
    } else {
        key = trim_copy(cleaned_line.substr(0, found_at));
        std::string right = trim_copy(cleaned_line.substr(found_at + used_delim.size()));
        value = right;
    }

    if (key.empty()) {
        errors.emplace_back(source, line_no, original_line);
        return;
    }

    auto* sec = find_section_items_mut(current_section);
    if (sec == nullptr) {
        throw Error(ErrorCode::NoSection, "No section: " + current_section);
    }

    key = option_xform(key);

    auto it = std::find_if(sec->begin(), sec->end(), [&](const OptionEntry& e) {
        return e.first == key;
    });
    if (it != sec->end()) {
        if (options_.strict) {
            throw LocatedError(
                ErrorCode::DuplicateOption,
                "duplicate option: " + key,
                source,
                line_no);
        }
        it->second = value;
    } else {
        sec->push_back({key, value});
    }

    current_option = key;
    multiline_accum.clear();
    if (value.has_value()) {
        multiline_accum.push_back(*value);
        in_multiline = true;
    } else {
        in_multiline = false;
    }
}

void Parser::join_multiline_values() {
}

void Parser::read_string(std::string_view text, std::string_view source) {
    std::istringstream iss{std::string(text)};
    std::string line;

    std::string current_section;
    std::string current_option;
    std::vector<std::string> multiline_accum;
    bool in_multiline = false;
    std::vector<ParsingError> errors;

    int line_no = 0;
    while (std::getline(iss, line)) {
        ++line_no;
        std::string cleaned = strip_comment(line);
        parse_line(
            line,
            cleaned,
            std::string(source),
            line_no,
            current_section,
            current_option,
            multiline_accum,
            in_multiline,
            errors);
    }

    if (in_multiline && !current_option.empty()) {
        finish_multiline(current_section, current_option, multiline_accum);
    }

    if (!errors.empty()) {
        ParsingError combined(errors.front().source(), errors.front().line(), errors.front().entries().front().text);
        for (std::size_t i = 1; i < errors.size(); ++i) {
            for (const auto& e : errors[i].entries()) {
                combined.append(e.line, e.text);
            }
        }
        throw combined;
    }

    join_multiline_values();
}

void Parser::read_file(std::string_view path) {
    std::ifstream ifs{std::string(path)};
    if (!ifs) {
        throw Error(ErrorCode::Parsing, "cannot open file: " + std::string(path));
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    read_string(oss.str(), path);
}

OptionValue Parser::get_raw(std::string_view section, std::string_view option) const {
    const auto* sec = find_section_items(section);
    const auto key = option_xform(option);

    if (sec != nullptr) {
        for (const auto& e : *sec) {
            if (e.first == key) {
                return e.second;
            }
        }
    }

    if (section != options_.default_section) {
        for (const auto& e : defaults_) {
            if (e.first == key) {
                return e.second;
            }
        }
    }

    if (sec == nullptr && section != options_.default_section) {
        throw Error(ErrorCode::NoSection, "No section: " + std::string(section));
    }

    throw Error(ErrorCode::NoOption, "No option '" + std::string(option) + "' in section: '" + std::string(section) + "'");
}

std::string Parser::interpolate(
    std::string_view section,
    std::string_view option,
    std::string_view value,
    int depth) const {
    if (options_.interpolation == InterpolationMode::None) {
        return std::string(value);
    }
    if (depth > options_.max_interpolation_depth) {
        throw Error(
            ErrorCode::InterpolationDepth,
            "Recursion limit exceeded in value substitution for option '" + std::string(option) + "' in section '" + std::string(section) + "'");
    }
    if (options_.interpolation == InterpolationMode::Basic) {
        return interpolate_basic(section, option, value, depth);
    }
    return interpolate_extended(section, option, value, depth);
}

std::string Parser::interpolate_basic(
    std::string_view section,
    std::string_view option,
    std::string_view value,
    int depth) const {
    std::string out;
    for (std::size_t i = 0; i < value.size();) {
        if (value[i] != '%') {
            out.push_back(value[i++]);
            continue;
        }
        if (i + 1 >= value.size()) {
            throw Error(ErrorCode::InterpolationSyntax, "'%' must be followed by '%' or '(' ");
        }
        if (value[i + 1] == '%') {
            out.push_back('%');
            i += 2;
            continue;
        }
        if (value[i + 1] != '(') {
            throw Error(ErrorCode::InterpolationSyntax, "'%' must be followed by '%' or '(' ");
        }

        auto close = value.find(")s", i + 2);
        if (close == std::string::npos) {
            throw Error(ErrorCode::InterpolationSyntax, "bad interpolation variable reference");
        }
        std::string key = option_xform(value.substr(i + 2, close - (i + 2)));
        auto replacement = get_raw(section, key);
        if (!replacement.has_value()) {
            i = close + 2;
            continue;
        }
        out += interpolate(section, key, *replacement, depth + 1);
        i = close + 2;
    }
    return out;
}

std::string Parser::interpolate_extended(
    std::string_view section,
    std::string_view option,
    std::string_view value,
    int depth) const {
    std::string out;
    for (std::size_t i = 0; i < value.size();) {
        if (value[i] != '$') {
            out.push_back(value[i++]);
            continue;
        }
        if (i + 1 >= value.size()) {
            throw Error(ErrorCode::InterpolationSyntax, "'$' must be followed by '$' or '{'");
        }
        if (value[i + 1] == '$') {
            out.push_back('$');
            i += 2;
            continue;
        }
        if (value[i + 1] != '{') {
            throw Error(ErrorCode::InterpolationSyntax, "'$' must be followed by '$' or '{'");
        }
        auto close = value.find('}', i + 2);
        if (close == std::string::npos) {
            throw Error(ErrorCode::InterpolationSyntax, "bad interpolation variable reference");
        }

        std::string token(value.substr(i + 2, close - (i + 2)));
        std::string ref_section(section);
        std::string ref_option;

        auto colon = token.find(':');
        if (colon == std::string::npos) {
            ref_option = option_xform(token);
        } else {
            ref_section = token.substr(0, colon);
            ref_option = option_xform(token.substr(colon + 1));
        }

        auto replacement = get_raw(ref_section, ref_option);
        if (!replacement.has_value()) {
            i = close + 1;
            continue;
        }
        out += interpolate(ref_section, ref_option, *replacement, depth + 1);
        i = close + 1;
    }
    return out;
}

std::string Parser::get(std::string_view section, std::string_view option) const {
    auto raw = get_raw(section, option);
    if (!raw.has_value()) {
        return "";
    }
    return interpolate(section, option, *raw, 1);
}

int Parser::get_int(std::string_view section, std::string_view option) const {
    const auto v = get(section, option);
    int out = 0;
    auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    if (ec != std::errc() || ptr != v.data() + v.size()) {
        throw Error(ErrorCode::Parsing, "Not an int: " + v);
    }
    return out;
}

double Parser::get_float(std::string_view section, std::string_view option) const {
    const auto v = get(section, option);
    char* end = nullptr;
    const double out = std::strtod(v.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        throw Error(ErrorCode::Parsing, "Not a float: " + v);
    }
    return out;
}

bool Parser::get_bool(std::string_view section, std::string_view option) const {
    const auto v = to_lower(get(section, option));
    if (v == "1" || v == "yes" || v == "true" || v == "on") {
        return true;
    }
    if (v == "0" || v == "no" || v == "false" || v == "off") {
        return false;
    }
    throw Error(ErrorCode::Parsing, "Not a boolean: " + v);
}

SectionItems Parser::items(std::string_view section, bool raw) const {
    const auto* sec = find_section_items(section);
    if (sec == nullptr) {
        throw Error(ErrorCode::NoSection, "No section: " + std::string(section));
    }

    SectionItems out = *sec;
    if (section != options_.default_section) {
        for (const auto& d : defaults_) {
            auto it = std::find_if(out.begin(), out.end(), [&](const OptionEntry& e) {
                return e.first == d.first;
            });
            if (it == out.end()) {
                out.push_back(d);
            }
        }
    }

    if (!raw) {
        for (auto& e : out) {
            if (e.second.has_value()) {
                e.second = interpolate(section, e.first, *e.second, 1);
            }
        }
    }

    return out;
}

void Parser::set(std::string_view section, std::string option, OptionValue value) {
    std::string sec_name(section);
    if (sec_name.empty()) {
        sec_name = options_.default_section;
    }

    SectionItems* sec = nullptr;
    if (sec_name == options_.default_section) {
        sec = &defaults_;
    } else {
        sec = find_section_items_mut(sec_name);
    }

    if (sec == nullptr) {
        throw Error(ErrorCode::NoSection, "No section: " + sec_name);
    }

    option = option_xform(option);
    auto it = std::find_if(sec->begin(), sec->end(), [&](const OptionEntry& e) {
        return e.first == option;
    });
    if (it == sec->end()) {
        sec->push_back({option, std::move(value)});
    } else {
        it->second = std::move(value);
    }
}

bool Parser::remove_option(std::string_view section, std::string_view option) {
    SectionItems* sec = nullptr;
    if (section.empty() || section == options_.default_section) {
        sec = &defaults_;
    } else {
        sec = find_section_items_mut(section);
    }
    if (sec == nullptr) {
        throw Error(ErrorCode::NoSection, "No section: " + std::string(section));
    }

    const auto key = option_xform(option);
    auto old_size = sec->size();
    sec->erase(std::remove_if(sec->begin(), sec->end(), [&](const OptionEntry& e) {
        return e.first == key;
    }), sec->end());
    return sec->size() != old_size;
}

bool Parser::remove_section(std::string_view section) {
    auto it = section_index_.find(std::string(section));
    if (it == section_index_.end()) {
        return false;
    }
    const std::size_t idx = it->second;
    sections_.erase(sections_.begin() + static_cast<std::ptrdiff_t>(idx));

    section_index_.clear();
    for (std::size_t i = 0; i < sections_.size(); ++i) {
        section_index_.insert({sections_[i].first, i});
    }
    return true;
}

std::string Parser::write_to_string(bool space_around_delimiters) const {
    std::ostringstream oss;
    const std::string delim = space_around_delimiters
        ? (" " + options_.delimiters.front() + " ")
        : options_.delimiters.front();

    if (!defaults_.empty()) {
        oss << '[' << options_.default_section << "]\n";
        for (const auto& e : defaults_) {
            if (e.second.has_value() || !options_.allow_no_value) {
                const std::string v = e.second.has_value() ? *e.second : "";
                if (e.first.find('[') == 0) {
                    throw Error(ErrorCode::InvalidWrite, "Cannot write key; begins with section pattern");
                }
                for (const auto& d : options_.delimiters) {
                    if (e.first.find(d) != std::string::npos) {
                        throw Error(ErrorCode::InvalidWrite, "Cannot write key; contains delimiter");
                    }
                }
                oss << e.first << delim << v << '\n';
            } else {
                oss << e.first << '\n';
            }
        }
        oss << '\n';
    }

    for (const auto& sec : sections_) {
        if (sec.first != kUnnamedSectionName) {
            oss << '[' << sec.first << "]\n";
        }
        for (const auto& e : sec.second) {
            if (e.second.has_value() || !options_.allow_no_value) {
                const std::string v = e.second.has_value() ? *e.second : "";
                if (e.first.find('[') == 0) {
                    throw Error(ErrorCode::InvalidWrite, "Cannot write key; begins with section pattern");
                }
                for (const auto& d : options_.delimiters) {
                    if (e.first.find(d) != std::string::npos) {
                        throw Error(ErrorCode::InvalidWrite, "Cannot write key; contains delimiter");
                    }
                }
                std::string folded = v;
                std::string::size_type p = 0;
                while ((p = folded.find('\n', p)) != std::string::npos) {
                    folded.replace(p, 1, "\n\t");
                    p += 2;
                }
                oss << e.first << delim << folded << '\n';
            } else {
                oss << e.first << '\n';
            }
        }
        oss << '\n';
    }

    return oss.str();
}

}  // namespace ini
