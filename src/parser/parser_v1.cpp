// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.

#include <exception.hpp>
#include <log.hpp>
#include <manifest.hpp>
#include <parameter.hpp>
#include <parser/common.hpp>
#include <rule.hpp>
#include <rule_processor/is_sqli.hpp>
#include <rule_processor/is_xss.hpp>
#include <rule_processor/phrase_match.hpp>
#include <rule_processor/regex_match.hpp>
#include <ruleset.hpp>
#include <ruleset_info.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using ddwaf::rule_processor::base;

namespace ddwaf::parser::v1 {

namespace {

condition::ptr parseCondition(parameter::map &rule, manifest &target_manifest,
    std::vector<PW_TRANSFORM_ID> transformers, ddwaf::object_limits limits)
{
    auto operation = at<std::string_view>(rule, "operation");
    auto params = at<parameter::map>(rule, "parameters");

    parameter::map options;
    std::shared_ptr<base> processor;
    if (operation == "phrase_match") {
        auto list = at<parameter::vector>(params, "list");

        std::vector<const char *> patterns;
        std::vector<uint32_t> lengths;

        patterns.reserve(list.size());
        lengths.reserve(list.size());

        for (auto &pattern : list) {
            if (pattern.type != DDWAF_OBJ_STRING) {
                throw ddwaf::parsing_error("phrase_match list item not a string");
            }

            patterns.push_back(pattern.stringValue);
            lengths.push_back((uint32_t)pattern.nbEntries);
        }

        processor = std::make_shared<rule_processor::phrase_match>(patterns, lengths);
    } else if (operation == "match_regex") {
        auto regex = at<std::string>(params, "regex");
        options = at<parameter::map>(params, "options", options);

        auto case_sensitive = at<bool>(options, "case_sensitive", false);
        auto min_length = at<int64_t>(options, "min_length", 0);
        if (min_length < 0) {
            throw ddwaf::parsing_error("min_length is a negative number");
        }

        processor =
            std::make_shared<rule_processor::regex_match>(regex, min_length, case_sensitive);
    } else if (operation == "is_xss") {
        processor = std::make_shared<rule_processor::is_xss>();
    } else if (operation == "is_sqli") {
        processor = std::make_shared<rule_processor::is_sqli>();
    } else {
        throw ddwaf::parsing_error("unknown processor: " + std::string(operation));
    }

    std::vector<condition::target_type> targets;
    auto inputs = at<parameter::vector>(params, "inputs");
    targets.reserve(inputs.size());
    for (const auto &input_param : inputs) {
        auto input = static_cast<std::string>(input_param);
        if (input.empty()) {
            throw ddwaf::parsing_error("empty address");
        }

        std::string root;
        std::string key_path;
        size_t pos = input.find(':', 0);
        if (pos == std::string::npos || pos + 1 >= input.size()) {
            root = input;
        } else {
            root = input.substr(0, pos);
            key_path = input.substr(pos + 1, input.size());
        }

        condition::target_type target;
        target.root = target_manifest.insert(root);
        target.name = std::move(root);
        if (!key_path.empty()) {
            target.key_path.emplace_back(key_path);
        }
        targets.emplace_back(std::move(target));
    }

    return std::make_shared<condition>(
        std::move(targets), std::move(transformers), std::move(processor), std::string{}, limits);
}

void parseRule(parameter::map &rule, ddwaf::ruleset_info &info, manifest &target_manifest,
    ddwaf::ruleset &rs, ddwaf::object_limits limits)
{
    auto id = at<std::string>(rule, "id");
    if (rs.rules.find(id) != rs.rules.end()) {
        DDWAF_WARN("duplicate rule %s", id.c_str());
        info.insert_error(id, "duplicate rule");
        return;
    }

    try {
        std::vector<PW_TRANSFORM_ID> rule_transformers;
        auto transformers = at<parameter::vector>(rule, "transformers", parameter::vector());
        for (const auto &transformer_param : transformers) {
            auto transformer = static_cast<std::string_view>(transformer_param);
            PW_TRANSFORM_ID transform_id = PWTransformer::getIDForString(transformer);
            if (transform_id == PWT_INVALID) {
                throw ddwaf::parsing_error("invalid transformer" + std::string(transformer));
            }
            rule_transformers.push_back(transform_id);
        }

        std::vector<condition::ptr> conditions;
        auto conditions_array = at<parameter::vector>(rule, "conditions");
        conditions.reserve(conditions_array.size());
        for (const auto &cond_param : conditions_array) {
            auto cond = static_cast<parameter::map>(cond_param);
            conditions.push_back(parseCondition(cond, target_manifest, rule_transformers, limits));
        }

        std::unordered_map<std::string, std::string> tags;
        for (auto &[key, value] : at<parameter::map>(rule, "tags")) {
            try {
                tags.emplace(key, std::string(value));
            } catch (const bad_cast &e) {
                throw invalid_type(std::string(key), e);
            }
        }

        if (tags.find("type") == tags.end()) {
            throw ddwaf::parsing_error("missing key 'type'");
        }

        auto rule_ptr = std::make_shared<ddwaf::rule>(
            std::string(id), at<std::string>(rule, "name"), std::move(tags), std::move(conditions));

        rs.insert_rule(rule_ptr);
        info.add_loaded();
    } catch (const std::exception &e) {
        DDWAF_WARN("failed to parse rule '%s': %s", id.c_str(), e.what());
        info.insert_error(id, e.what());
    }
}

} // namespace

void parse(parameter::map &ruleset, ruleset_info &info, ddwaf::ruleset &rs, object_limits limits)
{
    auto rules_array = at<parameter::vector>(ruleset, "events");
    rs.rules.reserve(rules_array.size());

    for (const auto &rule_param : rules_array) {
        try {
            auto rule = static_cast<parameter::map>(rule_param);
            parseRule(rule, info, rs.manifest, rs, limits);
        } catch (const std::exception &e) {
            DDWAF_WARN("%s", e.what());
            info.add_failed();
        }
    }

    if (rs.rules.empty()) {
        throw ddwaf::parsing_error("no valid rules found");
    }

    DDWAF_DEBUG("Loaded %zu rules out of %zu available in the ruleset", rs.rules.size(),
        rules_array.size());
}

} // namespace ddwaf::parser::v1
