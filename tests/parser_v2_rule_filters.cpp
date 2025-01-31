// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.

#include "test.h"

TEST(TestParserV2RuleFilters, ParseEmptyFilter)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(R"([{id: 1}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 0);
    EXPECT_EQ(exclusions.input_filters.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseFilterWithoutID)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(R"([{rules_target: [{rule_id: 2939}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 0);
    EXPECT_EQ(exclusions.input_filters.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseDuplicateUnconditionalRuleFilters)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939}]},{id: 1, rules_target: [{tags: {type: rule, category: unknown}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseUnconditionalRuleFilterTargetID)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(R"([{id: 1, rules_target: [{rule_id: 2939}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 0);
    EXPECT_EQ(exclusion.targets.size(), 1);

    const auto &target = exclusion.targets[0];
    EXPECT_EQ(target.type, parser::target_type::id);
    EXPECT_STR(target.rule_id, "2939");
    EXPECT_EQ(target.tags.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseUnconditionalRuleFilterTargetTags)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(R"([{id: 1, rules_target: [{tags: {type: rule, category: unknown}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 0);
    EXPECT_EQ(exclusion.targets.size(), 1);

    const auto &target = exclusion.targets[0];
    EXPECT_EQ(target.type, parser::target_type::tags);
    EXPECT_TRUE(target.rule_id.empty());
    EXPECT_EQ(target.tags.size(), 2);
    EXPECT_STR(target.tags.find("type")->second, "rule");
    EXPECT_STR(target.tags.find("category")->second, "unknown");
}

TEST(TestParserV2RuleFilters, ParseUnconditionalRuleFilterTargetPriority)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939, tags: {type: rule, category: unknown}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 0);
    EXPECT_EQ(exclusion.targets.size(), 1);

    const auto &target = exclusion.targets[0];
    EXPECT_EQ(target.type, parser::target_type::id);
    EXPECT_STR(target.rule_id, "2939");
    EXPECT_EQ(target.tags.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseUnconditionalRuleFilterMultipleTargets)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939},{tags: {type: rule, category: unknown}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 0);
    EXPECT_EQ(exclusion.targets.size(), 2);

    {
        const auto &target = exclusion.targets[0];
        EXPECT_EQ(target.type, parser::target_type::id);
        EXPECT_STR(target.rule_id, "2939");
        EXPECT_EQ(target.tags.size(), 0);
    }

    {
        const auto &target = exclusion.targets[1];
        EXPECT_EQ(target.type, parser::target_type::tags);
        EXPECT_TRUE(target.rule_id.empty());
        EXPECT_EQ(target.tags.size(), 2);
        EXPECT_STR(target.tags.find("type")->second, "rule");
        EXPECT_STR(target.tags.find("category")->second, "unknown");
    }
}

TEST(TestParserV2RuleFilters, ParseMultipleUnconditionalRuleFilters)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939}]},{id: 2, rules_target: [{tags: {type: rule, category: unknown}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 2);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    {
        const auto &exclusion_it = exclusions.rule_filters.find("1");
        EXPECT_STR(exclusion_it->first, "1");

        const auto &exclusion = exclusion_it->second;
        EXPECT_EQ(exclusion.conditions.size(), 0);
        EXPECT_EQ(exclusion.targets.size(), 1);

        const auto &target = exclusion.targets[0];
        EXPECT_EQ(target.type, parser::target_type::id);
        EXPECT_STR(target.rule_id, "2939");
        EXPECT_EQ(target.tags.size(), 0);
    }

    {
        const auto &exclusion_it = exclusions.rule_filters.find("2");
        EXPECT_STR(exclusion_it->first, "2");

        const auto &exclusion = exclusion_it->second;
        EXPECT_EQ(exclusion.conditions.size(), 0);
        EXPECT_EQ(exclusion.targets.size(), 1);

        const auto &target = exclusion.targets[0];
        EXPECT_EQ(target.type, parser::target_type::tags);
        EXPECT_TRUE(target.rule_id.empty());
        EXPECT_EQ(target.tags.size(), 2);
        EXPECT_STR(target.tags.find("type")->second, "rule");
        EXPECT_STR(target.tags.find("category")->second, "unknown");
    }
}

TEST(TestParserV2RuleFilters, ParseDuplicateConditionalRuleFilters)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939}], conditions: [{operator: match_regex, parameters: {inputs: [{address: arg1}], regex: .*}}]},{id: 1, rules_target: [{tags: {type: rule, category: unknown}}], conditions: [{operator: match_regex, parameters: {inputs: [{address: arg1}], regex: .*}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseConditionalRuleFilterSingleCondition)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939}], conditions: [{operator: match_regex, parameters: {inputs: [{address: arg1}], regex: .*}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 1);
    EXPECT_EQ(exclusion.targets.size(), 1);

    const auto &target = exclusion.targets[0];
    EXPECT_EQ(target.type, parser::target_type::id);
    EXPECT_STR(target.rule_id, "2939");
    EXPECT_EQ(target.tags.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseConditionalRuleFilterGlobal)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, conditions: [{operator: match_regex, parameters: {inputs: [{address: arg1}], regex: .*}}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 1);
    EXPECT_EQ(exclusion.targets.size(), 0);
}

TEST(TestParserV2RuleFilters, ParseConditionalRuleFilterMultipleConditions)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(
        R"([{id: 1, rules_target: [{rule_id: 2939}], conditions: [{operator: match_regex, parameters: {inputs: [{address: arg1}], regex: .*}}, {operator: match_regex, parameters: {inputs: [{address: arg2, key_path: [x]}], regex: .*}}, {operator: match_regex, parameters: {inputs: [{address: arg2, key_path: [y]}], regex: .*}}]}])");
    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 1);
    EXPECT_EQ(exclusions.input_filters.size(), 0);

    const auto &exclusion_it = exclusions.rule_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 3);
    EXPECT_EQ(exclusion.targets.size(), 1);

    const auto &target = exclusion.targets[0];
    EXPECT_EQ(target.type, parser::target_type::id);
    EXPECT_STR(target.rule_id, "2939");
    EXPECT_EQ(target.tags.size(), 0);
}
