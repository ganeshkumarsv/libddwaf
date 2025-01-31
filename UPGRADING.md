# Upgrading libddwaf

### Upgrading from `1.7.x` to `1.8.0`

Version `1.8.0` introduces the WAF builder, a new module with the ability to generate a new WAF instance from an existing one. This new module works transparently through the `ddwaf_update` function, which allows the user to update one, some or all of the following:
- The complete ruleset through the `rules` key.
- The `on_match` or `enabled` field of specific rules through the `rules_override` key.
- Exclusion filters through the `exclusions` key.
- Rule data through the `rules_data` key.

The WAF builder has a number of objectives:
- Provide a mechanism to generate and update the WAF as needed.
- Remove all existing mutexes.
- Remove all side-effects on running contexts.
- __Potentially__ provide efficiency gains: 
  - Avoiding the need to parse a whole ruleset on every update.
  - Reusing internal structures, objects and containers whenever possible.

With the introduction of `ddwaf_update`, the following functions have been deprecated and removed:
- `ddwaf_toggle_rules`
- `ddwaf_update_rule_data`
- `ddwaf_required_rule_data_ids`

The first two functions have been removed due to the added complexity of supporting multiple interfaces with a similar outcome but different inputs. On the other hand, the last function was simply removed in favour of letting the WAF handle unexpected rule data IDs more gracefully, however this function can be reintroduced later if deemed necessary.

Typically, the new interface will be used as follows on all instances:

```c
    ddwaf_handle old_handle = ddwaf_init(&ruleset, &config, &info);
    ddwaf_object_free(&ruleset);

    ddwaf_handle new_handle = ddwaf_update(old_handle, &update, &new_info);
    ddwaf_object_free(&update);
    if (new_handle != NULL) {
        ddwaf_destroy(old_handle);
    }
```

The `ddwaf_update` function returns a new `ddwaf_handle` which will be a valid pointer if the update succeeded, or `NULL` if there was nothing to update or there was an error. Creating contexts while calling `ddwaf_update` is, in theory, perfectly legal as well as destroying a handle while associated contexts are still in use, for example:

```c
    ddwaf_handle old_handle = ddwaf_init(&ruleset, &config, &info);
    ddwaf_object_free(&rule);

    ddwaf_context context = ddwaf_context_init(old_handle);

    ddwaf_handle new_handle = ddwaf_update(old_handle, &new_ruleset, &new_info);
    ddwaf_object_free(&new_rule);
    if (new_handle != NULL) {
        // Destroying the handle should not invalidate the context
        ddwaf_destroy(old_handle);
    }
    
    // Both the context and the handle are destroyed here
    ddwaf_context_destroy(context);
```
Note that the `ddwaf_update` function also has an optional input parameter for the `ruleset_info` structure, this will only provide useful diagnostics when the update provided contains new rules (within the `rules` key), also note that the `ruleset_info` should either be a fresh new structure or the previously used after calling `ddwaf_ruleset_info_free`.

Finally, you can call `ddwaf_init` with all previously mentioned keys, or a combination of them, however the `rules` key is mandatory. This does not apply to `ddwaf_update.

#### Notes on thread-safety

The thread-safety of any operations on the handle depends on whether they act on the ruleset or the builder itself, generally:
- Calling `ddwaf_update` concurrently, regardless of the handle, is never thread-safe.
- Calling `ddwaf_context_init` concurrently on the same handle is thread-safe.
- Calling `ddwaf_context_init` and `ddwaf_update` concurrently on the same handle is also thread-safe.

### Upgrading from `1.6.x` to `1.7.0`

There are no API changes in 1.7.0, however `ddwaf_handle` is now reference-counted and shared between the user and each `ddwaf_context`. This means that it is now possible to call `ddwaf_destroy` on a `ddwaf_handle` without invalidating any `ddwaf_context` in use instantiated from said `ddwaf_handle`. For example, the following snippet is now perfectly legal and will work as expected (note that any checks have been omitted for brevity):

```c
    ddwaf_handle handle = ddwaf_init(&rule, &config, NULL);
    ddwaf_object_free(&rule);

    ddwaf_context context = ddwaf_context_init(handle);

    // Destroying the handle should not invalidate the context
    ddwaf_destroy(handle);

    ...

    ddwaf_run(context, &parameter, NULL, LONG_TIME);

    // Both the context and the handle are destroyed here
    ddwaf_context_destroy(context);
```

To expand on the example above:
- `ddwaf_destroy` only destroys the `ddwaf_handle` if there are no more references to it, otherwise it relinquishes ownership to the rest of the contexts.
- If there is more than one valid context after the user calls `ddwaf_destroy`, each context will reduce the reference count on `ddwaf_context_destroy` until it reaches zero, at which point the `ddwaf_handle` will be freed.
- Once the user calls `ddwaf_destroy` on the `ddwaf_handle`, their reference becomes invalid and no more operations can be performed on it, including instantiating further contexts.

Note that this doesn't make `ddwaf_handle` universally thread-safe, for example, replacing an existing `ddwaf_handle` shared by multiple threads still requires synchronisation.

## Upgrading from `v1.4.0` to `1.5.0`

### Actions

The introduction of actions within the ruleset, through the `on_match` array, provides a generic mechanism to signal the user what the expected outcome of a match should be. This information is now provided to the user through `ddwaf_result`, which now has the following definition:

```c
struct _ddwaf_result
{
    /** Whether there has been a timeout during the operation **/
    bool timeout;
    /** Run result in JSON format **/
    const char* data;
    /** Actions array and its size **/
    struct {
        char **array;
        uint32_t size;
    } actions;
    /** Total WAF runtime in nanoseconds **/
    uint64_t total_runtime;
};
```

Actions are provided as a `char *` array containing `actions.size` items. This array is currently not null-terminated. The contents of this array and the array itself are also freed by `ddwaf_result_free`.

#### Return codes

As a consequence of the introduction of actions, return codes such as `DDWAF_MONITOR` or `DDWAF_BLOCK` are no longer meaningful, to address this:
- `DDWAF_MONITOR` has now been renamed to `DDWAF_MATCH`, this lets the user know that `ddwaf_result` contains meaningful data and possibly actions while making no statement regarding what the user should do with it.
- `DDWAF_BLOCK` has been removed.
- As a slightly unnecessary bonus, `DDWAF_GOOD` has been renamed to `DDWAF_OK` as it is more common to use `OK` than `GOOD` in return codes.

### Free function

In previous versions, the context is initialised with a free function in order to prevent objects provided through `ddwaf_run` from being freed by the WAF itself. In practice, this free function is always the same so it's not very useful to provide it over and over. For that reason it has now been moved to `ddwaf_config`.

As an example, in version `1.4.0`, the following code would provide `ddwaf_object_free` to the context during initialisation:

```c
ddwaf_config config{{0, 0, 0}, {NULL, NULL}};
ddwaf_handle handle = ddwaf_init(&rule, &config, &info);
...
ddwaf_context context = ddwaf_context_init(handle, ddwaf_object_free);
```

In version `1.5.0-alpha0`, the free function should be provided within `ddwaf_config`:
```c
ddwaf_config config{{0, 0, 0}, {NULL, NULL}, ddwaf_object_free};
ddwaf_handle handle = ddwaf_init(&rule, &config, &info);
...
ddwaf_context context = ddwaf_context_init(handle);
```

Alternatively, to completely disable the free function, it can be set to `NULL`:
```c
ddwaf_config config{{0, 0, 0}, {NULL, NULL}, NULL};
ddwaf_handle handle = ddwaf_init(&rule, &config, &info);
```

**Note that if the configuration pointer to the WAF is `NULL`, the default free function (`ddwaf_object_free`) will be used:**
```c
ddwaf_handle handle = ddwaf_init(&rule, NULL, &info);
```

### Version

In order to support version suffixes, such as `-alpha`, `-beta` or `-rc`, the `ddwaf_version` structure has been deprecated altogether. As a result `ddwaf_get_version` now returns a const string which should not be freed by the caller.

To summarize, the following code snippet:

```c
ddwaf_version version;
ddwaf_get_version(&version);
printf("ddwaf version: %d.%d.%d\n", version.major, version.minor, version.patch);
```

Can now be rewritten as follows:

```c
printf("ddwaf version: %s\n", ddwaf_get_version());
```
