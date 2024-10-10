/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OCRE_CONTAINER_RUNTIME_H
#define OCRE_CONTAINER_RUNTIME_H

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <ocre/fs/fs.h>
#include <ocre/sm/sm.h>

#include "wasm_export.h"

#include "ocre_container_runtime.h"
#include "../container_healthcheck/ocre_container_healthcheck.h"

#define OCRE_CR_DEBUG_ON        0   // Debug flag for container runtime (0: OFF, 1: ON)
#define MAX_CONTAINERS          10  // Maximum number of containers supported by the runtime !!! Can be configurable
#define FILE_PATH_MAX           256 // Maximum file path length
#define OCRE_CR_INIT_TIMEOUT    500 // Timeout to wait for the container registry to initialize

/**
 * @brief Structure containing the runtime arguments for a container runtime.
 */
typedef struct ocre_runtime_arguments_t {
    uint32_t size;                  ///< Size of the buffer.
    char *buffer;                   ///< Pointer to the buffer containing the WASM module.
    char error_buf[128];            ///< Buffer to store error messages.
    wasm_module_t module;           ///< Handle to the loaded WASM module.
    wasm_module_inst_t module_inst; ///< Handle to the instantiated WASM module.
    wasm_function_inst_t func;      ///< Handle to the function to be executed within the WASM module.
    wasm_exec_env_t exec_env;       ///< Execution environment for the WASM function.
    uint32_t stack_size;            ///< Stack size for the WASM module.
    uint32_t heap_size;             ///< Heap size for the WASM module.
} ocre_runtime_arguments_t;

/**
 * @brief Enum representing the permission types for containers.
 * NOT USED YET
 */
typedef enum {
    OCRE_CONTAINER_PERM_READ_ONLY,  ///< Container has read-only permissions.
    OCRE_CONTAINER_PERM_READ_WRITE, ///< Container has read and write permissions.
    OCRE_CONTAINER_PERM_EXECUTE     ///< Container has execute permissions.
} ocre_container_permissions_t;

/**
 * @brief Enum representing the possible status of the container runtime
 */
typedef enum {
    RUNTIME_STATUS_UNKNOWN,     ///< Status is unknown.
    RUNTIME_STATUS_INITIALIZED, ///< Runtime has been initialized.
    RUNTIME_STATUS_DESTROYED,   ///< Runtime has been destroyed
    RUNTIME_STATUS_ERROR        ///< An error occurred with the container.
} ocre_container_runtime_status_t;

/**
 * @brief Enum representing the possible status of a container.
 */
typedef enum {
    CONTAINER_STATUS_UNKNOWN,      ///< Status is unknown.
    CONTAINER_STATUS_CREATED,      ///< Container has been created.
    CONTAINER_STATUS_RUNNING,      ///< Container is currently running.
    CONTAINER_STATUS_STOPPED,      ///< Container has been stopped.
    CONTAINER_STATUS_DESTROYED,    ///< Container has been destroyed.
    CONTAINER_STATUS_UNRESPONSIVE, ///< Container is unresponsive. -> For Healthcheck
    CONTAINER_STATUS_ERROR,        ///< An error occurred with the container.
} ocre_container_status_t;

typedef struct ocre_container_runtime_init_arguments_t {
    uint32_t default_stack_size; ///< Stack size for the WASM module.
    uint32_t default_heap_size;  ///< Heap size for the WASM module.
    int maximum_containers;      ///< Maximum number of containers allowed.
    NativeSymbol *ocre_api_functions;
} ocre_container_init_arguments_t;

/**
 * @brief Structure representing the data associated with a container.
 */
typedef struct ocre_container_data_t {
    char name[16];        // <! Name of module (must be unique per installed instance)
    char sha256[70];      // <! Sha256 of file (to be used in file path)
    uint32_t stack_size;  ///< Stack size for the WASM module.
    uint32_t heap_size;   ///< Heap size for the WASM module.
    ocre_healthcheck WDT; ///< Watchdog timer for container health checking.
    int watchdog_interval;
    int timers;
} ocre_container_data_t;

/**
 * @brief Structure representing a container in the runtime.
 */
typedef struct ocre_container_t {
    ocre_runtime_arguments_t ocre_runtime_arguments;  ///< Runtime arguments for the container.
    ocre_container_status_t container_runtime_status; ///< Current status of the container.
    ocre_container_data_t ocre_container_data;        ///< Container-specific data.
} ocre_container_t;

/**
 * @brief Structure representing the context for container runtime operations.
 *
 * This structure is opaque to the user and is used internally by the runtime.
 * It should not be accessed or modified by the caller.
 */
typedef struct ocre_cs_ctx {
    ocre_container_t containers[MAX_CONTAINERS];
    int current_container_id;
    int download_count;
    struct k_sem initialized;
} ocre_cs_ctx;

/**
 * A function pointer type definition for container runtime callback.
 *
 * @param container_id The ID of the container.
 * @param status The status of the container.
 */
typedef void (*ocre_container_runtime_cb)(void);

/**
 * @brief Initializes the container runtime environment.
 *
 * Calling this method will initialize the container runtime.  The call will block until the runtime
 * is initalized or an error occurs.
 *
 * @param args Pointer to the runtime arguments structure.
 * @return Current status of the container runtime.
 */
ocre_container_runtime_status_t ocre_container_runtime_init(ocre_cs_ctx *ctx, ocre_container_init_arguments_t *args);

/**
 * @brief Destroys the container runtime environment.
 *
 * Calling this method will destroy the container runtime.  The call will block until the runtime
 * is destroyed or an error occurs.
 *
 * @param None.
 * @return Current status of the container runtime.
 */
ocre_container_status_t ocre_container_runtime_destroy(void);

/**
 * @brief Request the creation of a new container within the runtime.
 *
 * @param ctx Pointer to the container runtime context structure.
 * @param msg Pointer to the container data structure.
 * @param container_id Pointer to the container ID.
 * @param callback Optional callback function to be called when the container is created.
 *
 * @return Current status of the created container.
 */
ocre_container_status_t ocre_container_runtime_create_container(ocre_cs_ctx *ctx,
                                                                struct ocre_container_data_t *container_data,
                                                                int *container_id, ocre_container_runtime_cb callback);

/**
 * @brief Requests that the specified container is run.
 *
 * @param ctx Pointer to the container runtime context structure.
 * @param callback Optional callback function to be called when the container is created.
 *
 * @return Current status of the container after attempting to run it.
 */
ocre_container_status_t ocre_container_runtime_run_container(ocre_cs_ctx *ctx, int container_id,
                                                             ocre_container_runtime_cb callback);

/**
 * @brief Retrieves the current status of a specific container.
 *
 * @param ctx Pointer to the container runtime context structure.
 *
 * @return Current status of the specified container.
 */
ocre_container_status_t ocre_container_runtime_get_container_status(ocre_cs_ctx *ctx, int container_id);

/**
 * @brief Stops a running container.
 *
 * @param ctx Pointer to the container runtime context structure.
 * @param callback Optional callback function to be called when the container is created.
 *
 * @return Current status of the container after attempting to stop it.
 */
ocre_container_status_t ocre_container_runtime_stop_container(ocre_cs_ctx *ctx, int container_id,
                                                              ocre_container_runtime_cb callback);

/**
 * @brief Destroys and unloads a container from the runtime.
 *
 * @param ctx Pointer to the container runtime context structure.
 * @param msg Pointer to the container data structure.
 *
 * @return Current status of the container after attempting to destroy it.
 */
ocre_container_status_t ocre_container_runtime_destroy_container(ocre_cs_ctx *ctx, int container_id,
                                                                 ocre_container_runtime_cb callback);

/**
 * @brief Restarts a running container.
 *
 * @param ctx Pointer to the container runtime context structure.
 * @param callback Optional callback function to be called when the container is created.
 *
 * @return Current status of the container after attempting to restart it.
 */
ocre_container_status_t ocre_container_runtime_restart_container(ocre_cs_ctx *ctx, int container_id,
                                                                 ocre_container_runtime_cb callback);

#endif /* OCRE_CONTAINER_RUNTIME_H */