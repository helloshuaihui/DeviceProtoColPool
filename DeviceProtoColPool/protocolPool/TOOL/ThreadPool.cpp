#include "ThreadPool.h"

namespace TOOL {

ThreadPool::ThreadPool(size_t threadCount, size_t maxQueueSize) 
    : m_running(true), m_maxQueueSize(maxQueueSize) {
    // 如果未指定线程数或线程数为0，使用CPU核心数
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
        // 如果无法获取CPU核心数，默认使用4个线程
        if (threadCount == 0) {
            threadCount = 4;
        }
    }
    
    // 创建工作线程
    for (size_t i = 0; i < threadCount; ++i) {
        m_threads.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    // 唤醒所有线程
    m_cv.notify_all();
    // 唤醒所有等待队列不满的线程
    m_cvWaiting.notify_all();
    
    // 等待所有线程结束
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::worker() {
    while (m_running) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // 等待任务或停止信号
            m_cv.wait(lock, [this] {
                return !m_running || !m_tasks.empty();
            });
            
            // 如果停止且任务队列为空，退出
            if (!m_running && m_tasks.empty()) {
                return;
            }
            
            // 获取任务
            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
        }
        
        // 执行任务
        if (task) {
            task();
        }
        
        // 任务完成后，唤醒等待队列不满的线程
        if (m_maxQueueSize > 0) {
            m_cvWaiting.notify_one();
        }
    }
}

size_t ThreadPool::getThreadCount() const {
    return m_threads.size();
}

size_t ThreadPool::getTaskQueueSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

size_t ThreadPool::getMaxQueueSize() const {
    return m_maxQueueSize;
}

} // namespace TOOL