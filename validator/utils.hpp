// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#pragma once

#include "ddwaf.h"
#include <fstream>
#include <string>
#include <string_view>
#include <yaml-cpp/yaml.h>

namespace YAML {

class parsing_error : public std::exception {
public:
    explicit parsing_error(std::string_view what) : what_(what) {}
    [[nodiscard]] const char *what() const noexcept override { return what_.c_str(); }

protected:
    const std::string what_;
};

template <> struct as_if<ddwaf_object, void> {
    explicit as_if(const Node &node_) : node(node_) {}
    ddwaf_object operator()() const;
    const Node &node;
};

} // namespace YAML

std::string read_file(std::string_view filename);

namespace term {

enum class colour : unsigned {
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    magenta = 35,
    cyan = 36,
    white = 37,
    off = 39,
};

bool has_colour();
} // namespace term

std::ostream &operator<<(std::ostream &os, term::colour c);
