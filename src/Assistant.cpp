#include "Assistant.h"
#include <iostream>

namespace sba {

Assistant::~Assistant() noexcept {
    stop();
}

void Assistant::start() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (bRunning_) return;
        bRunning_ = true;
    }
    if (!input_ || !capture_) {
        HWND hwnd = FindWindowW(L"UnrealWindow", L"尘白禁区");
        if (!hwnd) {
            throw std::runtime_error("can not find window");
        }
        input_ = std::make_shared<Input>(hwnd);
        capture_ = std::make_shared<Capture>(hwnd);
    }
    taskThread_ = std::thread([this]() {
        try {
            taskLoop();
        } catch (const std::exception& e) {
            std::cerr << "[Assistant] taskLoop exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[Assistant] taskLoop unknown exception." << std::endl;
        }
    });
    capture_->start();
}

void Assistant::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!bRunning_) return;
        bRunning_ = false;
    }
    taskCv_.notify_all();
    if (taskThread_.joinable()) {
        taskThread_.join();
    }
    if (capture_) {
        capture_->stop();
        capture_ = nullptr;
    }
    input_ = nullptr;
}

void Assistant::addTask(std::shared_ptr<BaseTask> task) {
    if (!task) return;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskQueue_.push(std::move(task));
    }
    taskCv_.notify_one();
}

void Assistant::taskLoop() {
    while (true) {
        std::shared_ptr<BaseTask> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            taskCv_.wait(lock, [this]() { return !bRunning_ || !taskQueue_.empty(); });
            if (!bRunning_) break;

            if (!taskQueue_.empty()) {
                task = taskQueue_.front();
                taskQueue_.pop();
            }
        }

        if (task) {
            bool ok = task->run();
            std::cout << "[Assistant] Task \"" << task->name() << "\" (ID=" << task->taskID()
                      << ") finished with result: " << (ok ? "success" : "failed") << std::endl;
        }
    }
}

}  // namespace sba
