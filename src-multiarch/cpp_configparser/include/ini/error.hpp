#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace ini {

enum class ErrorCode {
    NoSection,
    NoOption,
    DuplicateSection,
    DuplicateOption,
    MissingSectionHeader,
    MultilineContinuation,
    InterpolationSyntax,
    InterpolationMissingOption,
    InterpolationDepth,
    InvalidWrite,
    Parsing,
};

class Error : public std::runtime_error {
public:
    explicit Error(ErrorCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}

    ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

class LocatedError : public Error {
public:
    LocatedError(
        ErrorCode code,
        std::string message,
        std::string source,
        int line
    )
        : Error(code, std::move(message)),
          source_(std::move(source)),
          line_(line) {}

    const std::string& source() const noexcept { return source_; }
    int line() const noexcept { return line_; }

private:
    std::string source_;
    int line_;
};

class ParsingError : public LocatedError {
public:
    struct Entry {
        int line;
        std::string text;
    };

    ParsingError(std::string source, int line, std::string text)
        : LocatedError(
              ErrorCode::Parsing,
              make_message(source),
              source,
              line) {
        append(line, std::move(text));
    }

    void append(int line, std::string text) {
        entries_.push_back({line, std::move(text)});
    }

    const std::vector<Entry>& entries() const noexcept { return entries_; }

private:
    static std::string make_message(const std::string& source) {
        return "Source contains parsing errors: '" + source + "'";
    }

    std::vector<Entry> entries_;
};

}  // namespace ini
