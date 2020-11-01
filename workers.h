#ifndef __WORKERS_H__
#define __WORKERS_H__

#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>
#include <thread>

class WorkersPool;

class Worker
{
public:
    Worker();
    virtual ~Worker() {}

    void async_step();
    void stop();

    void set_pool(WorkersPool* pool);

private:
    virtual bool do_step() = 0;
    void work_loop();

    bool go_;
    bool stop_;
    WorkersPool* pool_;

    std::condition_variable go_cond_;
    std::mutex go_mutex_;
    std::thread thread_;
};

class WorkersPool
{
public:
    ~WorkersPool();

    void add_worker(std::unique_ptr<Worker>& worker);

    void set_available(const Worker& worker);
    Worker* get_available();
    bool wait_all_available();

    bool is_failed();
    void set_failed(const Worker& worker);

    void stop_workers();

private:
    std::vector<std::unique_ptr<Worker> > workers_;

    std::deque<int> available_workers_;
    std::vector<int> failed_workers_;
    std::condition_variable available_cond_;
    std::mutex available_mutex_;
};

#endif
