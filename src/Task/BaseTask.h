#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace sba {

enum class TaskStatus {
    Pending,  // 等待中
    Running,  // 运行中
    Completed,  // 已完成
    Failed,  // 执行失败
    Cancelled  // 已取消
};

class Capture;
class Input;
class Assistant;

class BaseTask {
   public:
    BaseTask(const std::string& name, Assistant* assistant, int maxRetries = 3, int retryIntervalMs = 1000);
    virtual ~BaseTask() noexcept = default;

    virtual bool run();

    TaskStatus status() const noexcept { return status_; }
    const std::string& name() const noexcept { return name_; }
    int taskID() const noexcept { return id_; }
    std::shared_ptr<Capture> capture() const noexcept;
    std::shared_ptr<Input> input() const noexcept;

   protected:
    virtual bool execute() = 0;
    TaskStatus status_{TaskStatus::Pending};
    std::string name_;
    Assistant* assistant_{nullptr};
    int id_{0};
    int maxRetries_{3};
    int retryIntervalMs_{1000};

    static int generateUniqueID() noexcept;
    static std::atomic<int> idCounter_;
};

}  // namespace sba
