// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/validation_utils.h>
#include <aws/gamekit/gamelift/gamekit_gamelift.h>

using namespace Aws::Http;
using namespace Aws::Utils;
using namespace Aws::Utils::Json;
using namespace GameKit::ClientSettings::GameLift;
using namespace GameKit::GameLift;
using namespace GameKit::Utils::HttpClient;

#define DEFAULT_CLIENT_TIMEOUT_SECONDS  3
#define DEFAULT_RETRY_INTERVAL_SECONDS  5
#define DEFAULT_MAX_QUEUE_SIZE  256
#define DEFAULT_MAX_RETRIES 32
#define DEFAULT_RETRY_STRATEGY  0
#define DEFAULT_MAX_EXPONENTIAL_BACKOFF_THRESHOLD   32
#define DEFAULT_PAGINATION_SIZE 100

#pragma region Constructors/Deconstructor
GameLift::GameLift(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb)
{
    m_sessionManager = sessionManager;
    GameKit::AwsApiInitializer::Initialize(logCb, this);

    // Set default client settings
    m_clientSettings.ClientTimeoutSeconds = DEFAULT_CLIENT_TIMEOUT_SECONDS;
    m_clientSettings.RetryIntervalSeconds = DEFAULT_RETRY_INTERVAL_SECONDS;
    m_clientSettings.MaxRetryQueueSize = DEFAULT_MAX_QUEUE_SIZE;
    m_clientSettings.MaxRetries = DEFAULT_MAX_RETRIES;
    m_clientSettings.RetryStrategy = DEFAULT_RETRY_STRATEGY;
    m_clientSettings.MaxExponentialRetryThreshold = DEFAULT_MAX_EXPONENTIAL_BACKOFF_THRESHOLD;
    m_clientSettings.PaginationSize = DEFAULT_PAGINATION_SIZE;

    m_logCb = logCb;

    this->initializeClient();

    Logging::Log(logCb, Level::Info, "User Gameplay Data instantiated");
}

void GameLift::SetClientSettings(const GameLiftClientSettings& settings)
{
    m_clientSettings = settings;
    this->initializeClient();

    Logging::Log(m_logCb, Level::Info, "GameLift Client settings updated.");
}

GameLift::~GameLift()
{
    m_customHttpClient->StopRetryBackgroundThread();
    AwsApiInitializer::Shutdown(m_logCb, this);
    m_logCb = nullptr;
}
#pragma endregion

#pragma region Public Methods

unsigned int GameLift::CreateGameSession(GameLiftCreateSession gameLiftCreateSession, DISPATCH_RECEIVER_HANDLE createSessionReceiver, FuncCreateSessionResponseCallback createSessionCallback) {
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_MISSING;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_GAME_LIFT_BASE_URL];
    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::AddUserGameplayData() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    auto request = CreateHttpRequest(ToAwsString(uri), HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    JsonValue payload;
    gameLiftCreateSession.ToJson(payload);

    std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("CreateGameLiftSessionBody");
    Aws::String serialized = payload.View().WriteCompact();
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");
    request->SetContentLength(StringUtils::to_string(serialized.size()));

    createSessionCallback(createSessionReceiver, "1", "2");
    //RequestResult result = m_customHttpClient->MakeRequest(GameLiftOperationType::Write, false, userGameplayDataBundle.bundleName, "", request, HttpResponseCode::CREATED, m_clientSettings.MaxRetries);

    return GAMEKIT_ERROR_GAME_LIFT_CREATE_SESSION_ERROR;

}

#pragma endregion

#pragma region Private Methods
void GameLift::initializeClient()
{
    if (m_clientSettings.ClientTimeoutSeconds == 0)
    {
        m_clientSettings.ClientTimeoutSeconds = DEFAULT_CLIENT_TIMEOUT_SECONDS;
    }

    if (m_clientSettings.RetryIntervalSeconds == 0)
    {
        m_clientSettings.RetryIntervalSeconds = DEFAULT_RETRY_INTERVAL_SECONDS;
    }

    if (m_clientSettings.MaxRetryQueueSize == 0)
    {
        m_clientSettings.MaxRetryQueueSize = DEFAULT_MAX_QUEUE_SIZE;
    }

    if (m_clientSettings.MaxRetries == 0)
    {
        m_clientSettings.MaxRetries = DEFAULT_MAX_RETRIES;
    }

    if (m_clientSettings.RetryStrategy > 1)
    {
        // invalid value, set to default
        m_clientSettings.RetryStrategy = DEFAULT_RETRY_STRATEGY;
    }

    if (m_clientSettings.MaxExponentialRetryThreshold == 0)
    {
        m_clientSettings.MaxExponentialRetryThreshold = DEFAULT_MAX_EXPONENTIAL_BACKOFF_THRESHOLD;
    }

    if (m_clientSettings.PaginationSize == 0)
    {
        m_clientSettings.PaginationSize = DEFAULT_PAGINATION_SIZE;
    }

    // Low level client settings
    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.connectTimeoutMs = m_clientSettings.ClientTimeoutSeconds * 1000;
    clientConfig.httpRequestTimeoutMs = m_clientSettings.ClientTimeoutSeconds * 1000;
    clientConfig.requestTimeoutMs = m_clientSettings.ClientTimeoutSeconds * 1000;
    clientConfig.region = m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();

    auto lowLevelHttpClient = Aws::Http::CreateHttpClient(clientConfig);

    // High level settings for custom client
    auto strategyBuilder = [&]()
    {
        StrategyType strategyType = (StrategyType)m_clientSettings.RetryStrategy;
        std::shared_ptr<IRetryStrategy> retryLogic;
        switch (strategyType)
        {
        case StrategyType::ExponentialBackoff:
            retryLogic = std::make_shared<ExponentialBackoffStrategy>(m_clientSettings.MaxExponentialRetryThreshold, m_logCb);
            break;
        case StrategyType::ConstantInterval:
            retryLogic = std::make_shared<ConstantIntervalStrategy>();
            break;
        }

        return retryLogic;
    };

    // Auth token setter
    std::function<void(std::shared_ptr<HttpRequest>)> authSetter =
        std::bind(&GameLift::setAuthorizationHeader, this, std::placeholders::_1);

    // Build custom client with retry logic
    auto retryStrategy = strategyBuilder();
    m_customHttpClient = std::make_shared<GameLiftHttpClient>(
        lowLevelHttpClient, authSetter, m_clientSettings.RetryIntervalSeconds, retryStrategy, m_clientSettings.MaxRetryQueueSize, m_logCb);
}

void GameLift::setAuthorizationHeader(std::shared_ptr<HttpRequest> request)
{
    std::string value = "Bearer " + m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    request->SetHeaderValue(HEADER_AUTHORIZATION, ToAwsString(value));
}

void GameLift::setPaginationLimit(std::shared_ptr<HttpRequest> request, unsigned int paginationLimit)
{
    request->AddQueryStringParameter(LIMIT_KEY.c_str(), StringUtils::to_string(paginationLimit));
}

void GameLift::setHttpClient(std::shared_ptr<Aws::Http::HttpClient> httpClient)
{
    m_customHttpClient->SetLowLevelHttpClient(httpClient);
}
#pragma endregion
