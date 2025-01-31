// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.

#include "test.h"

TEST(TestParserV2InputFilters, ParseFilterWithoutID)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;

    auto object = readRule(R"([{inputs: [{address: http.client_ip}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 0);
    EXPECT_EQ(exclusions.input_filters.size(), 0);
}

TEST(TestParserV2InputFilters, ParseDuplicateFilters)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;
    manifest.insert("http.client_ip");
    manifest.insert("usr.id");

    auto object = readRule(
        R"([{id: 1, inputs: [{address: http.client_ip}]}, {id: 1, inputs: [{address: usr.id}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 0);
    EXPECT_EQ(exclusions.input_filters.size(), 1);
}

TEST(TestParserV2InputFilters, ParseNoConditionsOrTargets)
{
    ddwaf::manifest manifest;
    ddwaf::object_limits limits;
    manifest.insert("http.client_ip");
    manifest.insert("usr.id");

    auto object = readRule(R"([{id: 1, inputs: [{address: http.client_ip}]}])");

    auto exclusions_array = static_cast<parameter::vector>(parameter(object));
    auto exclusions = parser::v2::parse_filters(exclusions_array, manifest, limits);
    ddwaf_object_free(&object);

    EXPECT_EQ(exclusions.rule_filters.size(), 0);
    EXPECT_EQ(exclusions.input_filters.size(), 1);

    const auto &exclusion_it = exclusions.input_filters.begin();
    EXPECT_STR(exclusion_it->first, "1");

    const auto &exclusion = exclusion_it->second;
    EXPECT_EQ(exclusion.conditions.size(), 0);
    EXPECT_EQ(exclusion.targets.size(), 0);
    EXPECT_TRUE(exclusion.filter);
}

// TODO more tests
