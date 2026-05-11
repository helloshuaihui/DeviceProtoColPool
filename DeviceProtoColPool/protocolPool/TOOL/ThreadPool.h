#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace TOOL {

class ThreadPool {
public:
    // 构造函数：指定线程数量和最大任务队列数量
    // threadCount: 线程数量，默认使用CPU核心数
    // maxQueueSize: 最大任务队列数量，0表示无限制
    explicit ThreadPool(size_t threadCount = 0, size_t maxQueueSize = 0);
    
    // 析构函数：停止所有线程
    ~ThreadPool();
    
    // 禁用拷贝构造和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // 提交任务到队列，如果队列满则等待
    template<typename F, typename... Args>
    void submit(F&& f, Args&&... args);
    
    // 尝试提交任务，如果队列满则返回false
    template<typename F, typename... Args>
    bool trySubmit(F&& f, Args&&... args);
    
    // 停止线程池
    void stop();
    
    // 获取线程数量
    size_t getThreadCount() const;
    
    // 获取任务队列当前大小
    size_t getTaskQueueSize() const;
    
    // 获取最大任务队列数量
    size_t getMaxQueueSize() const;

private:
    // 工作线程函数
    void worker();
    
    std::vector<std::thread> m_threads;      // 线程列表
    std::queue<std::function<void()>> m_tasks; // 任务队列
    
    mutable std::mutex m_mutex;              // 互斥锁（mutable支持const成员函数加锁）
    std::condition_variable m_cv;            // 条件变量
    std::condition_variable m_cvWaiting;     // 队列不满条件变量
    
    std::atomic<bool> m_running;             // 运行标志
    size_t m_maxQueueSize;                   // 最大任务队列数量，0表示无限制
};

template<typename F, typename... Args>
void ThreadPool::submit(F&& f, Args&&... args) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // 如果限制了队列大小，需要等待队列不满
        if (m_maxQueueSize > 0) {
            m_cvWaiting.wait(lock, [this] {
                return m_tasks.size() < m_maxQueueSize;
            });
        }
        
        m_tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }
    m_cv.notify_one();
}

template<typename F, typename... Args>
bool ThreadPool::trySubmit(F&& f, Args&&... args) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 如果限制了队列大小且队列已满，返回false
        if (m_maxQueueSize > 0 && m_tasks.size() >= m_maxQueueSize) {
            return false;
        }
        
        m_tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }
    m_cv.notify_one();
    return true;
}

} // namespace TOOL