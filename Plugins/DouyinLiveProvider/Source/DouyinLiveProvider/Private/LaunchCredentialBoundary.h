#pragma once
#include <Windows.h>

namespace gwlaunch {
struct InputBoundary { bool Pipe = false; bool DataPending = false; };
enum class MissingCredential { TokenMissing, EntryBypassed, PipeFailed };
inline MissingCredential ClassifyMissingCredential(bool launcherMarker, bool stdinFlag)
{
    if (stdinFlag) return MissingCredential::PipeFailed;
    return launcherMarker ? MissingCredential::TokenMissing : MissingCredential::EntryBypassed;
}
// Observe the pipe boundary without reading any bytes or exposing their size.
inline InputBoundary ObserveInput(HANDLE input)
{
    DWORD available = 0;
    const bool pipe = input && input != INVALID_HANDLE_VALUE
        && PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr) != 0;
    return {pipe, pipe && available > 0};
}
}
