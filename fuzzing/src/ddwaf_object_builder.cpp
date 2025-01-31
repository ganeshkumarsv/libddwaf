// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog (https://www.datadoghq.com/).
// Copyright 2021 Datadog, Inc.

#include "ddwaf_object_builder.hpp"
#include "helpers.hpp"
#include <cstdio>
#include <cstring>

struct Data {
    const uint8_t *bytes;
    size_t size;
    size_t position = 0;
};

uint8_t popSize(Data *data)
{
    if (data->position >= data->size) {
        return 0;
    }

    uint8_t size = data->bytes[data->position];
    data->position++;

    return size;
}

uint8_t popSelector(Data *data, uint8_t maximumValue) { return popSize(data) % maximumValue; }

void popBytes(Data *data, void *dest, uint8_t n)
{
    if (data->position + n - 1 >= data->size) {
        data->position = data->size;
        return;
    }

    memcpy(dest, data->bytes + data->position, n);

    data->position += n;
}

void pop_string(Data *data, ddwaf_object *object)
{
    if (data->position >= data->size) {
        ddwaf_object_stringl(object, "", 0);
        return;
    }

    char *result = (char *)(data->bytes + data->position);
    size_t size = 0;

    // reserve this useless char for end of string
    uint8_t ENDOFSTRING = 31; // unit separator

    while (data->position < data->size && data->bytes[data->position] != ENDOFSTRING) {
        size++;
        data->position++;
    }

    if (data->position < data->size) {
        data->position++; // here, data->bytes[data->position] == ENDOFSTRING
    }

    // sometimes, send NULL
    if (size == 3 && result[0] == 6 && result[1] == 6 && result[2] == 6) {
        ddwaf_object_stringl(object, NULL, size);
    }

    ddwaf_object_stringl(object, result, size);
}

uint64_t popUnsignedInteger(Data *data)
{
    uint64_t result = 0;

    popBytes(data, &result, 8);

    return result;
}

int64_t popInteger(Data *data)
{
    int64_t result = 0;

    popBytes(data, &result, 8);

    return result;
}

uint16_t popUInt16(Data *data)
{
    uint16_t result = 0;

    popBytes(data, &result, 2);

    return result;
}
uint8_t popByte(Data *data)
{
    uint8_t result = 0;

    popBytes(data, &result, 1);

    return result;
}

ddwaf_object create_object(Data *data, size_t deep);

void build_map(Data *data, ddwaf_object *object, size_t deep)
{
    ddwaf_object_map(object);
    ddwaf_object key, item;

    uint8_t size = popSize(data);

    if (deep == 0) {
        size = 0;
    }

    for (uint8_t i = 0; i < size && data->position < data->size; i++) {
        pop_string(data, &key);
        item = create_object(data, deep - 1);
        if (!ddwaf_object_map_addl(object, key.stringValue, key.nbEntries, &item)) {
            ddwaf_object_free(&item);
        };
        ddwaf_object_free(&key);
    }
}

void build_array(Data *data, ddwaf_object *object, size_t deep)
{
    ddwaf_object_array(object);

    uint8_t size = popSize(data);
    ddwaf_object item;

    if (deep == 0) {
        size = 0;
    }

    for (uint8_t i = 0; i < size && data->position < data->size; i++) {
        item = create_object(data, deep - 1);
        if (!ddwaf_object_array_add(object, &item)) {
            ddwaf_object_free(&item);
        }
    }
}

ddwaf_object create_object(Data *data, size_t deep)
{
    ddwaf_object result;
    uint8_t selector = popSelector(data, 5);

    switch (selector) {
    case 4:
        ddwaf_object_unsigned(&result, popUnsignedInteger(data));
        break;
    case 3:
        ddwaf_object_signed(&result, popInteger(data));
        break;
    case 2:
        pop_string(data, &result);
        break;
    case 1:
        build_array(data, &result, deep);
        break;
    case 0:
        build_map(data, &result, deep);
        break;
    }

    return result;
}

void log(const char *message, bool verbose)
{
    if (verbose) {
        fprintf(stderr, "%s\n", message);
    }
}

ddwaf_object build_object(
    const uint8_t *bytes, size_t size, bool verbose, bool fuzzTimeout, size_t *timeLeftInMs)
{
    Data data;

    data.bytes = bytes;
    data.size = size;

#pragma GCC diagnostic ignored "-Wunused-parameter"
    ddwaf_log_cb cb = [](DDWAF_LOG_LEVEL level, const char *function, const char *file,
                          unsigned line, const char *message, uint64_t message_len) {};

    uint8_t selector = popSize(&data);

    if (selector == 255) {
        ddwaf_set_log_cb(cb, DDWAF_LOG_TRACE);
        log("DDWAF_LOG_TRACE", verbose);
    } else if (selector == 254) {
        ddwaf_set_log_cb(cb, DDWAF_LOG_DEBUG);
        log("DDWAF_LOG_DEBUG", verbose);
    } else if (selector == 253) {
        ddwaf_set_log_cb(cb, DDWAF_LOG_INFO);
        log("DDWAF_LOG_INFO", verbose);
    } else if (selector == 252) {
        ddwaf_set_log_cb(cb, DDWAF_LOG_WARN);
        log("DDWAF_LOG_WARN", verbose);
    } else if (selector == 251) {
        ddwaf_set_log_cb(cb, DDWAF_LOG_ERROR);
        log("DDWAF_LOG_ERROR\n", verbose);
    } else {
        ddwaf_set_log_cb(nullptr, DDWAF_LOG_DEBUG);
        log("DDWAF_LOG_<nocb>", verbose);
    }

    if (popUInt16(&data) == 16896) { // all 65000 => reset
        log("Reload rules", verbose);
        // TODO
        /*        pw_clearRule(RULE_NAME);*/
        /*initPowerWaf();*/
    }

    if (popUInt16(&data) == 16896) { // all 65000 => reset all
        log("clear all", verbose);
        /*        pw_clearAll();*/
        /*initPowerWaf();*/
    }

    if (fuzzTimeout) {
        *timeLeftInMs = (size_t)popUInt16(&data);
    } else {
        *timeLeftInMs = 200000; // 200ms
    }

    ddwaf_object result;
    build_map(&data, &result, 30);

    if (verbose) {
        print_object(result);
    }

    return result;
}
