// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.

#pragma once

#include <memory>

#include <ddwaf.h>
#include <event.hpp>
#include <optional>
#include <rule.hpp>
#include <config.hpp>
#include <utils.h>
#include <obfuscator.hpp>

namespace ddwaf
{

class context
{
public:
    context(const ddwaf::ruleset &ruleset, const ddwaf::config &config):
        ruleset_(ruleset), config_(config),
        store_(ruleset_.manifest, config_.free_fn)
    {
        status_cache_.reserve(ruleset_.rules.size());
    }

    context(const context&) = delete;
    context& operator=(const context&) = delete;
    context(context&&) = default;
    context& operator=(context&&) = delete;
    ~context() = default;

    DDWAF_RET_CODE run(const ddwaf_object&, optional_ref<ddwaf_result> res, uint64_t);

protected:
    bool run_collection(const std::string& name,
        const ddwaf::rule_ref_vector& collection,
        event_serializer& serializer,
        ddwaf::timer& deadline);

    bool is_first_run() const { return status_cache_.empty(); }

    const ddwaf::ruleset &ruleset_;
    const ddwaf::config &config_;
    ddwaf::object_store store_;

    // If we have seen a match, the value will be true, if the value is present
    // and false it means we executed the rule and it did not match.
    std::unordered_map<rule::index_type, bool> status_cache_;
};

}
