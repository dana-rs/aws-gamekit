// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The C interface for the GameLift library.
 */

#pragma once

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/model/account_info.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/logging.h>

 /**
  * @brief GameKitGameLift instance handle created by calling GameKitGameLiftInstanceCreateWithSessionManager()
 */
typedef void* GAMEKIT_GAME_LIFT_INSTANCE_HANDLE;

extern "C"
{
    /**
     * @brief Creates an GameLift instance, which can be used to access the GameLift API.
     *
     * @details Make sure to call GameKitGameLiftInstanceRelease to destroy the returned object when finished with it.
     *
     * @param sessionManager Pointer to a session manager object which manages tokens and configuration.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameLift instance.
    */
    GAMEKIT_API GAMEKIT_GAME_LIFT_INSTANCE_HANDLE GameKitGameLiftInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb);

    /**
     * @brief Destroys the passed in GameLift instance.
     *
     * @param GameLiftInstance Pointer to GameKit::GameLift instance created with GameKitGameLiftInstanceCreateWithSessionManager()
    */
    GAMEKIT_API void GameKitGameLiftInstanceRelease(GAMEKIT_GAME_LIFT_INSTANCE_HANDLE GameLiftInstance);
}
