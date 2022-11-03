// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK Forward Declarations
namespace Aws {
  namespace Utils {
    namespace Json { class JsonValue; }
  }
}

namespace GameKit
{
    /**
     * @brief Settings for the User Gameplay Data API client.
     *
     */
    struct GameLiftClientSettings
    {
        /**
         * @brief Connection timeout in seconds for the internal HTTP client. Default is 3. Uses default if set to 0.
         */
        unsigned int ClientTimeoutSeconds;

        /**
         * @brief Seconds to wait between retries. Default is 5. Uses default value if set to 0.
         */
        unsigned int RetryIntervalSeconds;

        /**
         * @brief Maximum length of custom http client request queue size. Once the queue is full, new requests will be dropped. Default is 256. Uses default if set to 0.
         */
        unsigned int MaxRetryQueueSize;

        /**
         * @brief Maximum number of times to retry a request before dropping it. Default is 32. Uses default if set to 0.
         */
        unsigned int MaxRetries;

        /**
         * @brief Retry strategy to use. Use 0 for Exponential Backoff, 1 for Constant Interval. Default is 0.
         */
        unsigned int RetryStrategy;

        /**
         * @brief Maximum retry threshold for Exponential Backoff. Forces a retry even if exponential backoff is set to a greater value. Default is 32. Uses default if set to 0.
         */
        unsigned int MaxExponentialRetryThreshold;

        /**
         * @brief Number of items to retrieve when executing paginated calls such as Get All Data. Default is 100. Uses default if set to 0.
         */
        unsigned int PaginationSize;
    };
}
