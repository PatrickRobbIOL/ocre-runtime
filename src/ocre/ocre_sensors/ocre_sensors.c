/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <ocre/ocre.h>
LOG_MODULE_DECLARE(ocre_sensors, OCRE_LOG_LEVEL);

#include "ocre_sensors.h"

#define DEVICE_NODE      DT_PATH(devices)
#define HAS_DEVICE_NODES DT_NODE_EXISTS(DEVICE_NODE) && DT_PROP_LEN(DEVICE_NODE, device_list) > 1
#if (CONFIG_OCRE_SENSORS) && (HAS_DEVICE_NODES)
#define EXTRACT_LABELS(node_id, prop, idx) DT_PROP_BY_PHANDLE_IDX_OR(node_id, prop, idx, label, "undefined")
static const char *sensor_discovery_names[] = {
        DT_FOREACH_PROP_ELEM_SEP(DEVICE_NODE, device_list, EXTRACT_LABELS, (, ))};
static int sensor_names_count = sizeof(sensor_discovery_names) / sizeof(sensor_discovery_names[0]);
#else
static const char *sensor_discovery_names[] = {};
static int sensor_names_count = 0;
#endif

typedef struct {
    const struct device *device;
    ocre_sensor_t info;
    bool in_use;
} ocre_sensor_internal_t;

static ocre_sensor_internal_t sensors[CONFIG_MAX_SENSORS] = {0};
static int sensor_count = 0;
static char sensor_names[CONFIG_MAX_SENSORS][CONFIG_MAX_SENSOR_NAME_LENGTH];

static int set_opened_channels(const struct device *dev, sensor_channel_t *channels) {
    if (!channels) {
        LOG_ERR("Channels array is NULL");
        return -1;
    }

    int count = 0;
    struct sensor_value value = {};

    for (int channel = 0; channel < SENSOR_CHAN_ALL && count < CONFIG_MAX_CHANNELS_PER_SENSOR; channel++) {
        if (sensor_channel_get(dev, channel, &value) == 0) {
            channels[count] = channel;
            count++;
        }
    }
    return count;
}

int ocre_sensors_init(wasm_exec_env_t exec_env) {
    memset(sensors, 0, sizeof(sensors));
    memset(sensor_names, 0, sizeof(sensor_names));
    sensor_count = 0;
    return 0;
}

int ocre_sensors_open(wasm_exec_env_t exec_env, ocre_sensor_handle_t handle) {
    if (handle < 0 || handle >= sensor_count || !sensors[handle].in_use) {
        LOG_ERR("Invalid sensor handle: %d", handle);
        return -1;
    }

    if (!device_is_ready(sensors[handle].device)) {
        LOG_ERR("Device %s is not ready", sensors[handle].info.sensor_name);
        return -2;
    }

    return 0;
}

int ocre_sensors_discover(wasm_exec_env_t exec_env) {
    const struct device *dev = NULL;
    size_t device_count = z_device_get_all_static(&dev);

    if (!dev) {
        LOG_ERR("Device list is NULL. Possible memory corruption!");
        return -1;
    }
    if (device_count == 0) {
        LOG_ERR("No static devices found");
        return -1;
    }

    LOG_INF("Total static devices found: %zu", device_count);

    for (size_t i = 0; i < device_count && sensor_count < CONFIG_MAX_SENSORS; i++) {
        if (!dev[i].name) {
            LOG_ERR("Device %zu has NULL name, skipping!", i);
            continue;
        }

        LOG_INF("Checking device: %s", dev[i].name);

        // Check if device name is in the sensor discovery list
        bool sensor_found = false;
        for (int j = 0; j < sensor_names_count; j++) {
            if (strcmp(dev[i].name, sensor_discovery_names[j]) == 0) {
                sensor_found = true;
                break;
            }
        }
        if (!sensor_found) {
            LOG_WRN("Skipping device, not in sensor list: %s", dev[i].name);
            continue;
        }

        // if (!device_is_ready(&dev[i])) {
        //     LOG_WRN("Device %s is not ready, skipping", dev[i].name);
        //     continue;
        // }

        const struct sensor_driver_api *api = (const struct sensor_driver_api *)dev[i].api;
        if (!api || !api->channel_get) {
            LOG_WRN("Device %s does not support sensor API or channel_get, skipping", dev[i].name);
            continue;
        }

        // Ensure we don't exceed sensor limit
        if (sensor_count >= CONFIG_MAX_SENSORS) {
            LOG_WRN("Max sensor limit reached, skipping device: %s", dev[i].name);
            continue;
        }

        // Initialize the sensor
        ocre_sensor_internal_t *sensor = &sensors[sensor_count];
        sensor->device = &dev[i];
        sensor->in_use = true;
        sensor->info.handle = sensor_count;

        strncpy(sensor_names[sensor_count], dev[i].name, CONFIG_MAX_SENSOR_NAME_LENGTH - 1);
        sensor_names[sensor_count][CONFIG_MAX_SENSOR_NAME_LENGTH - 1] = '\0';
        sensor->info.sensor_name = sensor_names[sensor_count];

        // Get supported channels
        sensor->info.num_channels = set_opened_channels(&dev[i], sensor->info.channels);
        if (sensor->info.num_channels <= 0) {
            LOG_WRN("Device %s does not have opened channels, skipping", dev[i].name);
            continue;
        }

        LOG_INF("Device has %d channels", sensor->info.num_channels);
        sensor_count++;
    }

    LOG_INF("Discovered %d sensors", sensor_count);
    return sensor_count;
}

int ocre_sensors_get_handle(wasm_exec_env_t exec_env, int sensor_id) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
        return -1;
    }
    return sensors[sensor_id].info.handle;
}

int ocre_sensors_get_name(wasm_exec_env_t exec_env, int sensor_id, char *buffer, int buffer_size) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use || !buffer) {
        return -1;
    }

    int name_len = strlen(sensors[sensor_id].info.sensor_name);
    if (name_len >= buffer_size) {
        return -2;
    }

    strncpy(buffer, sensors[sensor_id].info.sensor_name, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return name_len;
}

int ocre_sensors_get_channel_count(wasm_exec_env_t exec_env, int sensor_id) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
        return -1;
    }
    return sensors[sensor_id].info.num_channels;
}

int ocre_sensors_get_channel_type(wasm_exec_env_t exec_env, int sensor_id, int channel_index) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use || channel_index < 0 ||
        channel_index >= sensors[sensor_id].info.num_channels) {
        return -1;
    }
    return sensors[sensor_id].info.channels[channel_index];
}

int ocre_sensors_read(wasm_exec_env_t exec_env, int sensor_id, int channel_type) {
    if (sensor_id < 0 || sensor_id >= sensor_count || !sensors[sensor_id].in_use) {
        return -1;
    }

    const struct device *dev = sensors[sensor_id].device;
    struct sensor_value value = {};

    if (sensor_sample_fetch(dev) < 0) {
        return -1;
    }

    if (sensor_channel_get(dev, channel_type, &value) < 0) {
        return -1;
    }

    return value.val1 * 1000 + value.val2 / 1000;
}