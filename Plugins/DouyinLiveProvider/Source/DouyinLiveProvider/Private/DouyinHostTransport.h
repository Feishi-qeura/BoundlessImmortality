#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "Containers/Queue.h"

/** Anonymous inherited pipes, with bounded queues and no UI/network listener. */
class FDouyinHostTransport final : public FRunnable
{
public:
    ~FDouyinHostTransport() { Shutdown(); }
    bool Launch(const FString& Executable,const FString& EnvironmentArgs=FString());
    bool Enqueue(const FString& Json);
    bool Dequeue(FString& Json,double* ReceivedAt=nullptr);
    bool HasFailed() const { return bFailed.Load(); }
    void Shutdown();
    uint32 Run() override;
    void Stop() override { bStop.Store(true); }
private:
    TUniquePtr<FRunnableThread> Thread;
    FProcHandle Process;
    void* ReadParent=nullptr;
    void* WriteParent=nullptr;
    struct FInboundFrame {FString Json;double ReceivedAt=0;};
    TQueue<FInboundFrame,EQueueMode::Spsc> Incoming;
    TQueue<FString,EQueueMode::Spsc> Outgoing;
    TAtomic<int32> IncomingCount{0},OutgoingCount{0};
    TAtomic<int64> IncomingBytes{0},OutgoingBytes{0};
    TAtomic<bool> bStop{false},bFailed{false};
};
