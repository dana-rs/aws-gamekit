// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/gamelift/exports.h>
#include <aws/gamekit/gamelift/gamekit_gamelift.h>

using namespace GameKit::Logger;
using namespace GameKit::GameLift;

GAMEKIT_GAME_LIFT_INSTANCE_HANDLE GameKitGameLiftInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb)
{
    Logging::Log(logCb, Level::Info, "GameLift Instance Create with default settings.");
    GameKit::Authentication::GameKitSessionManager* sessMgr = (GameKit::Authentication::GameKitSessionManager*)sessionManager;
    GameLift* gameLift = new GameLift(sessMgr, logCb);

    return (GameKit::GameKitFeature*)gameLift;
}

void GameKitSetUserGameplayDataClientSettings(GAMEKIT_GAME_LIFT_INSTANCE_HANDLE GameLiftInstance, GameKit::GameLiftClientSettings settings)
{
    ((GameLift*)((GameKit::GameKitFeature*)GameLiftInstance))->SetClientSettings(settings);
}

