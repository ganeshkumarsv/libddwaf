// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.

#pragma once

#include <ddwaf.h>
#include <memory>
#include <re2/re2.h>
#include <string_view>

namespace ddwaf {

// For now this class only services as an inmutable instance of an obfuscator
// which provides a verdict regarding whether to obfuscate or not. Eventually
// the objective would be to directly pass events to the obfuscator and have it
// obfuscate as required.

class obfuscator {
public:
    explicit obfuscator(std::string_view key_regex_str = std::string_view(),
        std::string_view value_regex_str = std::string_view());
    [[nodiscard]] bool is_sensitive_key(std::string_view key) const;
    [[nodiscard]] bool is_sensitive_value(std::string_view value) const;

    static constexpr std::string_view redaction_msg{"<Redacted>"};

protected:
    static constexpr std::string_view default_key_regex_str{
        R"((p(ass)?w(or)?d|pass(_?phrase)?|secret|(api_?|private_?|public_?)key)|token|consumer_?(id|key|secret)|sign(ed|ature)|bearer|authorization)"};

    std::unique_ptr<re2::RE2> key_regex{nullptr};
    std::unique_ptr<re2::RE2> value_regex{nullptr};
};

} // namespace ddwaf
