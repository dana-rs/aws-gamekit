// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/utils/json/JsonSerializer.h>

// GameKit
#include <aws/gamekit/gamelift/gamekit_gamelift_models.h>

void GameKit::GameLiftCreateSession::ToJson(Aws::Utils::Json::JsonValue& json) const
{
    for (size_t i = 0; i < numKeys; i++)
    {
        json.WithString(gamePropertyKeys[i], gamePropertyValues[i]);
    }
}