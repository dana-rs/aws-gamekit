// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <sstream>
#include <string>

// AWS SDK
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>

// GameKit
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/utils/ticker.h>
#include <aws/gamekit/core/utils/validation_utils.h>
#include <aws/gamekit/gamelift/exports.h>
#include <aws/gamekit/gamelift/gamekit_gamelift_client.h>
#include <aws/gamekit/gamelift/gamekit_gamelift_models.h>

using namespace GameKit::Logger;

namespace GameKit
{

    class IGameLiftFeature
    {
    public:
        IGameLiftFeature() {};
        virtual ~IGameLiftFeature() {};

        virtual unsigned int CreateGameSession(GameLiftCreateSession gameLiftCreateSession, DISPATCH_RECEIVER_HANDLE createSessionReceiver, FuncCreateSessionResponseCallback createSessionCallback) = 0;


    };

    namespace GameLift
    {
        static const Aws::String HEADER_AUTHORIZATION = "Authorization";
        static const Aws::String LIMIT_KEY = "limit";

        class GAMEKIT_API GameLift : public GameKitFeature, public IGameLiftFeature
        {
        private:
            Authentication::GameKitSessionManager* m_sessionManager;
            std::shared_ptr<GameLiftHttpClient> m_customHttpClient;
            GameLiftClientSettings m_clientSettings;

            void initializeClient();
            void setAuthorizationHeader(std::shared_ptr<Aws::Http::HttpRequest> request);
            void setPaginationLimit(std::shared_ptr<Aws::Http::HttpRequest> request, unsigned int paginationLimit);

            /**
             * @brief Sets the Low Level Http client to use for this feature. Should be used for testing only.
             *
             * @param httpClient Shared pointer to an http client for this feature to use.
             */
            void setHttpClient(std::shared_ptr<Aws::Http::HttpClient> httpClient);

        public:

            /**
             * @brief Constructor, obtains resource handles and initializes clients with default settings.
             *
             * @param logCb Callback function for logging information and errors.
             * @param sessionManager GameKitSessionManager instance that manages tokens and configuration.
            */
            GameLift(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb);

            /**
             * @brief Applies the settings to the internal clients. Should be called immediately after the constructor and before any other API calls.
             *
             * @param settings Client settings.
            */
            void SetClientSettings(const GameLiftClientSettings& settings);

            virtual ~GameLift() override;

            virtual unsigned int CreateGameSession(GameLiftCreateSession gameLiftCreateSession, DISPATCH_RECEIVER_HANDLE createSessionReceiver, FuncCreateSessionResponseCallback createSessionCallback) override;
        };
    }
}
