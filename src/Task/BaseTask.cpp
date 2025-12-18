#include "BaseTask.h"
#include <chrono>
#include <thread>

namespace sba {

std::atomic<int> BaseTask::idCounter_{1};

int BaseTask::generateUniqueID() noexcept {
    return idCounter_.fetch_add(1, std::memory_order_relaxed);
}

bool BaseTask::run() {
    if (status_ == TaskStatus::Cancelled) {
        return true;
    }

    status_ = TaskStatus::Running;
    bool success = false;

    for (int attempt = 0; attempt <= maxRetries_; ++attempt) {
        if (status_ == TaskStatus::Cancelled) {
            return true;
        }
        success = execute();
        if (success) {
            status_ = TaskStatus::Completed;
            break;
        }
        if (attempt < maxRetries_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retryIntervalMs_));
        }
    }

    if (!success) {
        status_ = TaskStatus::Failed;
    }
    return success;
}

}  // namespace sba
