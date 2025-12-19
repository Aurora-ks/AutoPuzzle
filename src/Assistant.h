#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "Controller/Capture.h"
#include "Controller/Input.h"
#include "Task/BaseTask.h"

namespace sba {

class Assistant {
   public:
    Assistant() = default;
    ~Assistant() noexcept;

    void start();
    void stop();
    void addTask(std::shared_ptr<BaseTask> task);

    std::shared_ptr<Capture> capture() const noexcept { return capture_; }
    std::shared_ptr<Input> input() const noexcept { return input_; }

   private:
    void taskLoop();

    bool bRunning_{false};
    std::shared_ptr<Input> input_;
    std::shared_ptr<Capture> capture_;
    std::mutex mutex_;
    std::condition_variable taskCv_;
    std::queue<std::shared_ptr<BaseTask>> taskQueue_;
    std::thread taskThread_;
};

}  // namespace sba
