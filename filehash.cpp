#include "hash-calculator.h"
#include "workers.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>

#include <unistd.h>

class HashWorker: public Worker
{
public:
    HashWorker(size_t block_size, std::unique_ptr<IHashCalculator>& hc);
    ~HashWorker() {}

    void set_nblock(size_t nblock) { nblock_ = nblock; }
    std::vector<char>& get_buf() { return buf_; }

    static void flush_ready_blocks();

private:
    static std::mutex result_mutex_;
    static std::map<size_t, std::vector<unsigned char> > result_;
    static size_t expected_block_;

    virtual bool do_step();
    std::unique_ptr<IHashCalculator> hc_;
    std::vector<char> buf_;
    size_t nblock_;
};

std::mutex HashWorker::result_mutex_;
std::map<size_t, std::vector<unsigned char> > HashWorker::result_;
size_t HashWorker::expected_block_ = 0;

HashWorker::HashWorker(size_t block_size, std::unique_ptr<IHashCalculator>& hc):
    hc_(std::move(hc))
{
    buf_.resize(block_size);
}

bool HashWorker::do_step()
{
    if (hc_->init() == 0 ||
        hc_->update(&buf_[0], buf_.size()) == 0 ||
        hc_->final() == 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(result_mutex_);
    result_.insert(std::pair<size_t, std::vector<unsigned char> >(nblock_, hc_->get_hash()));
    return true;
}

void HashWorker::flush_ready_blocks()
{
    std::lock_guard<std::mutex> lock(result_mutex_);
    for (std::map<size_t, std::vector<unsigned char> >::iterator i = result_.begin(); i != result_.end() && i->first == expected_block_; i = result_.begin())
    {
//        std::cout << i->first << " ";
        for (auto& b : i->second)
            std::cout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int)b;
        std::cout << std::setw(0) << std::dec << std::endl;

        ++expected_block_;
        result_.erase(i);
    }
}

class FileProcessor
{
public:
    FileProcessor(size_t block_size, const char* hash_type);

    bool process_file();

private:
    std::string hash_type_;
    WorkersPool workers_;
    long int nprocs_;
    size_t current_block_;
};

FileProcessor::FileProcessor(size_t block_size, const char* hash_type):
    hash_type_(hash_type),
    nprocs_(sysconf(_SC_NPROCESSORS_ONLN)),
    current_block_(0)
{
    if (nprocs_ < 1)
        nprocs_ = 1;

    for (int i = 0; i < nprocs_; ++i)
    {
        std::unique_ptr<IHashCalculator> hc(IHashCalculator::create(hash_type));
        if (!hc)
            exit(1); // unable to continue

        std::unique_ptr<Worker> hw(new HashWorker(block_size, hc));
        workers_.add_worker(hw);
    }
}

bool FileProcessor::process_file()
{
    bool result = true;
    for (current_block_ = 0; ; ++current_block_)
    {
        HashWorker* const worker = static_cast<HashWorker*>(workers_.get_available());
        if (worker == nullptr) // some worker has failed
        {
            result = false;
            break;
        }

        std::vector<char>& buf = worker->get_buf();
        std::cin.read(&buf[0], buf.size());
        const size_t cnt = std::cin.gcount();
        if (cnt != 0)
        {
            if (cnt < buf.size())
                memset(&buf[cnt], 0, buf.size() - cnt);
            worker->set_nblock(current_block_);
            worker->async_step();
        }

        HashWorker::flush_ready_blocks();

        if (std::cin.eof())
            break;

        if (std::cin.fail())
        {
            result = false;
            std::cerr << "unable to read from file " << strerror(errno) << std::endl;
            break;
        }
    }
    if (!workers_.wait_all_available())
        result = false;

    HashWorker::flush_ready_blocks();

    return result;
}

int main(int argc, char** argv)
{
    static const char HASH_TYPE[] = "MD5";
// TODO: take type from command line
    size_t blk_size = 1024*1024;

    if (argc > 1)
    {
        if (sscanf(argv[1], "%zu", &blk_size) != 1)
            std::cerr << "block size " << argv[1] << " is not valid. default " << blk_size << " will be used" << std::endl;
    }

    FileProcessor fproc(blk_size, HASH_TYPE);
    if (!fproc.process_file())
        return 1;

    return 0;
}
