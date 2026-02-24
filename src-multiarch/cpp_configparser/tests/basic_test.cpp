#include "ini/parser.hpp"

#include <cassert>
#include <iostream>

int main() {
    ini::ParseOptions opt;
    opt.interpolation = ini::InterpolationMode::Basic;

    ini::Parser p(opt);
    p.read_string(R"ini(
[DEFAULT]
base = /tmp

[app]
name = demo
path = %(base)s/%(name)s
enabled = yes
count = 7
pi = 3.14
)ini");

    assert(p.get("app", "name") == "demo");
    const std::string resolved_path = p.get("app", "path");
    std::cerr << "resolved_path=" << resolved_path << "\n";
    assert(resolved_path == "/tmp/demo");
    assert(p.get_bool("app", "enabled") == true);
    assert(p.get_int("app", "count") == 7);

    const double pi = p.get_float("app", "pi");
    assert(pi > 3.13 && pi < 3.15);

    p.set("app", "new_key", std::string("value"));
    assert(p.get("app", "new_key") == "value");

    const std::string out = p.write_to_string();
    assert(out.find("[app]") != std::string::npos);
    assert(out.find("new_key = value") != std::string::npos);

    std::cout << "ini_configparser_basic_test passed\n";
    return 0;
}
