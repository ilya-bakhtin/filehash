#include "workers.h"

#include <iostream>

Worker::Worker():
    go_(false),
    stop_(false),
    pool_(nullptr),
    thread_(&Worker::work_loop, this)
{
}

void Worker::async_step()
{
    std::lock_guard<std::mutex> lock(go_mutex_);
    go_ = true;
    go_cond_.notify_one();
}

void Worker::stop()
{
    {
        std::lock_guard<std::mutex> lock(go_mutex_);
        stop_ = true;
        go_cond_.notify_one();
    }
    thread_.join();
}

void Worker::set_pool(WorkersPool* pool)
{
    pool_ = pool;
}

void Worker::work_loop()
{
    for (;!stop_;)
    {
        std::unique_lock<std::mutex> lock(go_mutex_);
        while (!go_ && !stop_)
            go_cond_.wait(lock);

        if (go_)
        {
            if (do_step())
            {
                go_ = false;
                pool_->set_available(*this);
            }
            else
            {
                stop_ = true;
                pool_->set_failed(*this);
            }
        }
    }
}

WorkersPool::~WorkersPool()
{
    stop_workers();
}

void WorkersPool::add_worker(std::unique_ptr<Worker>& worker)
{
    workers_.push_back(std::move(worker));
    workers_.back()->set_pool(this);
    set_available(*workers_.back());
}

void WorkersPool::set_available(const Worker& worker)
{
    for (size_t n = 0; n < workers_.size(); ++n)
    {
        if (&worker == workers_[n].get())
        {
            std::lock_guard<std::mutex> lock(available_mutex_);
            available_workers_.push_back(n);
            available_cond_.notify_one();
            return;
        }
    }
}

void WorkersPool::set_failed(const Worker& worker)
{
    for (size_t n = 0; n < workers_.size(); ++n)
    {
        if (&worker == workers_[n].get())
        {
            std::lock_guard<std::mutex> lock(available_mutex_);
            failed_workers_.push_back(n);
            available_cond_.notify_one();
            return;
        }
    }
}

bool WorkersPool::is_failed()
{
    std::lock_guard<std::mutex> lock(available_mutex_);
    return !failed_workers_.empty();
}

Worker* WorkersPool::get_available()
{
    std::unique_lock<std::mutex> lock(available_mutex_);

    while (available_workers_.empty() && failed_workers_.empty())
        available_cond_.wait(lock);

    if (!failed_workers_.empty())
        return nullptr;

    const size_t first = available_workers_.front();
    available_workers_.pop_front();
    return workers_[first].get();
}

bool WorkersPool::wait_all_available()
{
    std::unique_lock<std::mutex> lock(available_mutex_);

    while (failed_workers_.empty() && available_workers_.size() != workers_.size())
        available_cond_.wait(lock);

    return failed_workers_.empty();
}

void WorkersPool::stop_workers()
{
    for (auto& wrk : workers_)
        wrk->stop();
}
