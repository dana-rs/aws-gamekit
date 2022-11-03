// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/gamelift/gamekit_gamelift_client.h>

using namespace GameKit::Utils::HttpClient;
using namespace GameKit::Utils::Serialization;
using namespace GameKit::GameLift;

#pragma region GameLift Public Methods
bool GameKit::GameLift::OperationTimestampCompare(const std::shared_ptr<IOperation> lhs, const std::shared_ptr<IOperation> rhs)
{
    return lhs->Timestamp < rhs->Timestamp;
}
#pragma endregion

#pragma region GameLiftOperation Public Methods
bool GameLiftOperation::TrySerializeBinary(std::ostream& os, const std::shared_ptr<IOperation> operation, FuncLogCallback logCb)
{
    auto gameliftOperation = std::static_pointer_cast<GameLiftOperation>(operation);

    return GameLiftOperation::TrySerializeBinary(os, gameliftOperation, logCb);
}

bool GameLiftOperation::TrySerializeBinary(std::ostream& os, const std::shared_ptr<GameLiftOperation> operation, FuncLogCallback logCb)
{
    try
    {
        os.exceptions(std::ostream::failbit); // throw on failure

        BinWrite(os, operation->Type);
        BinWrite(os, operation->Bundle);
        BinWrite(os, operation->ItemKey);
        BinWrite(os, operation->MaxAttempts);
        BinWrite(os, operation->ExpectedSuccessCode);
        BinWrite(os, operation->Timestamp.count());

        return TrySerializeRequestBinary(os, operation->Request, logCb);
    }
    catch (const std::ios_base::failure& failure)
    {
        std::string message = "Could not serializeGameLiftOperation, " + std::string(failure.what());
        Logging::Log(logCb, Level::Error, message.c_str());
    }

    return false;
}

bool GameLiftOperation::TryDeserializeBinary(std::istream& is, std::shared_ptr<IOperation>& outOperation, FuncLogCallback logCb)
{
    auto outGameplayOperation = std::static_pointer_cast<GameLiftOperation>(outOperation);

    if (GameKit::GameLift::GameLiftOperation::TryDeserializeBinary(is, outGameplayOperation, logCb))
    {
        outOperation = std::static_pointer_cast<IOperation>(outGameplayOperation);
        return true;
    }

    return false;
}

bool GameLiftOperation::TryDeserializeBinary(std::istream& is, std::shared_ptr<GameLiftOperation>& outOperation, FuncLogCallback logCb)
{
    GameLiftOperationType type;
    std::string bundle;
    std::string item;

    int maxAttempts;
    Aws::Http::HttpResponseCode expectedCode;
    long long milliseconds;

    try
    {
        is.exceptions(std::istream::failbit); // throw on failure

        BinRead(is, type);
        BinRead(is, bundle);
        BinRead(is, item);
        BinRead(is, maxAttempts);
        BinRead(is, expectedCode);
        BinRead(is, milliseconds);

        std::shared_ptr<Aws::Http::HttpRequest> request;
        if (TryDeserializeRequestBinary(is, request))
        {
            outOperation = std::make_shared<GameLiftOperation>(type, bundle, item, request, expectedCode, maxAttempts, std::chrono::milliseconds(milliseconds));

            return true;
        }
    }
    catch (const std::ios_base::failure& failure)
    {
        std::string message = "Could not deserialize GameLiftOperation, " + std::string(failure.what());
        Logging::Log(logCb, Level::Error, message.c_str());
    }

    return false;
}
#pragma endregion

#pragma region GameLiftHttpClient Public Methods
RequestResult GameLiftHttpClient::MakeRequest(GameLiftOperationType operationType,
    bool isAsync,
    const char* bundle,
    const char* itemKey,
    std::shared_ptr<Aws::Http::HttpRequest> request,
    Aws::Http::HttpResponseCode successCode,
    unsigned int maxAttempts,
    CallbackContext callbackContext,
    ResponseCallback successCallback,
    ResponseCallback failureCallback)
{
    std::shared_ptr<IOperation> operation = std::make_shared<GameLiftOperation>(
        operationType, bundle, itemKey, request, successCode, maxAttempts);

    operation->CallbackContext = callbackContext;
    operation->SuccessCallback = successCallback;
    operation->FailureCallback = failureCallback;

    auto result = this->makeOperationRequest(operation, isAsync, false);

    std::string message = "GameLiftHttpClient::MakeRequest with operation " + std::to_string((int)operationType) +
        ", async " + std::to_string(isAsync) + ", bundle " + bundle + ", item " + itemKey + result.ToString();
    Logging::Log(m_logCb, Level::Verbose, message.c_str());

    return result;
}
#pragma endregion

#pragma region GameLiftHttpClient Private/Protected Methods
void GameLiftHttpClient::filterQueue(OperationQueue* queue, OperationQueue* filtered)
{
    Logging::Log(m_logCb, Level::Verbose, "GameLiftHttpClient::FilterQueue");
    std::map<std::string, std::deque<GameLiftOperation*>> temp;
    unsigned int operationsDiscarded = 0;

    // Sort queue based on timestamp. Order is important for User Gameplay Data filtering
    std::sort(queue->begin(), queue->end(), OperationTimestampCompare);

    for (auto operationIt = queue->begin(); operationIt != queue->end(); ++operationIt)
    {
        GameLiftOperation* operation = static_cast<GameLiftOperation*>((*operationIt).get());

        auto& queueWithSameKey = temp[operation->OperationUniqueKey];

        if (queueWithSameKey.empty())
        {
            queueWithSameKey.push_back(operation);
        }
        else
        {
            auto& previousOperation = queueWithSameKey.back();

            // If keys don't match, keep both.
            // Some rare coincidence caused two operations to be mapped to the same queue
            if (operation->Bundle != previousOperation->Bundle ||
                operation->ItemKey != previousOperation->ItemKey)
            {
                Logging::Log(m_logCb, Level::Warning, "GameLiftOperation key mismatch, keeping both operations.");
                queueWithSameKey.push_back(operation);
            }

            // If this is an item-level operation, most recent one is kept
            // If this is a bundle-level operation, or global,
            // and if most recent is delete, keep delete, else keep both

            if (!operation->ItemKey.empty() && !previousOperation->ItemKey.empty())
            {
                Logging::Log(m_logCb, Level::Verbose, "Discarding previous item operation, newer operation overwrites data.");
                previousOperation->Discard = true;
                queueWithSameKey.pop_back();
            }
            else if (operation->Type == GameLiftOperationType::Delete)
            {
                Logging::Log(m_logCb, Level::Verbose, "Discarding previous bundle operation, newer operation overwrites data.");
                previousOperation->Discard = true;
                queueWithSameKey.pop_back();
            }

            queueWithSameKey.push_back(operation);
        }
    }

    // Enqueue non discarded operations
    for (auto& operation : *queue)
    {
        if (!operation->Discard)
        {
            filtered->push_back(operation);
        }
    }

    std::string message = "GameLiftHttpClient::FilterQueue. Discarded " + std::to_string(operationsDiscarded) + " operations.";
    Logging::Log(m_logCb, Level::Info, message.c_str());
}

bool GameLiftHttpClient::shouldEnqueueWithUnhealthyConnection(const std::shared_ptr<IOperation> operation) const
{
    auto ugpdOperation = static_cast<const GameLiftOperation*>(operation.get());

    return ugpdOperation->Type != GameLiftOperationType::Get;
}

bool GameLiftHttpClient::isOperationRetryable(const std::shared_ptr<IOperation> operation,
    std::shared_ptr<const Aws::Http::HttpResponse> response) const
{
    auto ugpdOperation = static_cast<const GameLiftOperation*>(operation.get());

    bool attemptsExhausted = ugpdOperation->MaxAttempts != OPERATION_ATTEMPTS_NO_LIMIT && ugpdOperation->Attempts > ugpdOperation->MaxAttempts;
    bool isResponseRetryable = BaseHttpClient::isResponseCodeRetryable(response->GetResponseCode());

    std::string message = "GameLiftHttpClient::IsOperationRetryable: Attempts exhausted " + std::to_string(attemptsExhausted) +
        ", Type " + std::to_string(int(ugpdOperation->Type)) + ", IsResponseCodeRetryable " + std::to_string(isResponseRetryable);
    Logging::Log(m_logCb, Level::Verbose, message.c_str());

    return !attemptsExhausted &&
        ugpdOperation->Type != GameLiftOperationType::Get &&
        isResponseRetryable;
}
#pragma endregion
