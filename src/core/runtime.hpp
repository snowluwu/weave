#pragma once
#include "pch.h"

namespace weave {
    class CallScope {
    public:
        CallScope();
        ~CallScope();
        explicit operator bool() const { return entered; }
        CallScope(const CallScope&) = delete;
        CallScope& operator=(const CallScope&) = delete;
    private:
        bool entered;
    };

    class WorkScope {
    public:
        WorkScope();
        ~WorkScope();
    };

    bool SubmitWork(PTP_SIMPLE_CALLBACK callback, void* context, PTP_CALLBACK_ENVIRON environment);
    bool HasUdpIo();
    bool HasAsyncConnections();
}
