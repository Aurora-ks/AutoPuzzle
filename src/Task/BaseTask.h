#pragma once

#include <atomic>
#include <string>

namespace sba {

enum class TaskStatus {
    Pending,  // 等待中
    Running,  // 运行中
    Completed,  // 已完成
    Failed,  // 执行失败
    Cancelled  // 已取消
};

class BaseTask {
   public:
    BaseTask(const std::string& name, int maxRetries = 3, int retryIntervalMs = 1000)
        : name_(name), maxRetries_(maxRetries), retryIntervalMs_(retryIntervalMs) {
        id_ = generateUniqueID();
    }
    virtual ~BaseTask() noexcept = default;

    virtual bool run();

    inline TaskStatus status() const noexcept { return status_; }
    inline const std::string& name() const noexcept { return name_; }
    inline int taskID() const noexcept { return id_; }

   protected:
    virtual bool execute() = 0;
    TaskStatus status_{TaskStatus::Pending};
    std::string name_;
    int id_{0};
    int maxRetries_{3};
    int retryIntervalMs_{1000};

    static int generateUniqueID() noexcept;
    static std::atomic<int> idCounter_;
};

}  // namespace sba
