#include "hash-calculator.h"

#include <openssl/evp.h>

#include <iostream>
#include <memory>
#include <vector>

class EvpHashCalculator: public IHashCalculator
{
public:
    EvpHashCalculator(const char* name);
    virtual ~EvpHashCalculator();

    virtual int init();
    virtual int update(const void* d, size_t cnt);
    virtual int final();

    virtual const std::vector<unsigned char>& get_hash() const;

    static IHashCalculator* create(const char* name);
private:
    EvpHashCalculator(const EvpHashCalculator& other);
    EvpHashCalculator(EvpHashCalculator&& other);
    EvpHashCalculator& operator=(const EvpHashCalculator& other);
    EvpHashCalculator& operator=(EvpHashCalculator&& other);

    EVP_MD_CTX* mdctx_;
    std::string name_;
    const EVP_MD* const md_;
    std::vector<unsigned char> hash_;
};

EvpHashCalculator::EvpHashCalculator(const char* name):
    mdctx_(nullptr),
    name_(name),
    md_(EVP_get_digestbyname(name))
{
    if (md_ == nullptr)
    {
        std::cerr << "Unknown hash name \"" << name << "\"" << std:: endl;
        return;
    }

    hash_.resize(EVP_MD_size(md_));

    mdctx_ = EVP_MD_CTX_new();

    if (mdctx_ == nullptr)
        std::cerr << "Unable to allocate EVP MD CTX" << std:: endl;
}

EvpHashCalculator::~EvpHashCalculator()
{
    EVP_MD_CTX_free(mdctx_);
}

int EvpHashCalculator::init()
{
    int res = EVP_MD_CTX_reset(mdctx_);
    if (res == 0)
        std::cerr << "EVP_MD_CTX_reset failed" << std::endl;
    else
    {
        res = EVP_DigestInit_ex(mdctx_, md_, nullptr);
        if (res == 0)
            std::cerr << "EVP_DigestInit_ex failed" << std::endl;
    }
    return res;
}

int EvpHashCalculator::update(const void* d, size_t cnt)
{
    const int res = EVP_DigestUpdate(mdctx_, d, cnt);
    if (res == 0)
        std::cerr << "EVP_DigestUpdate failed" << std::endl;
    return res;
}

int EvpHashCalculator::final()
{
    unsigned int len;
    const int res = EVP_DigestFinal_ex(mdctx_, &hash_[0], &len);
    if (res == 0)
        std::cerr << "EVP_DigestFinal_ex failed" << std::endl;
    return res;
}

const std::vector<unsigned char>& EvpHashCalculator::get_hash() const
{
    return hash_;
}

IHashCalculator* IHashCalculator::create(const char* name)
{
    return EvpHashCalculator::create(name);
}

IHashCalculator* EvpHashCalculator::create(const char* name)
{
    std::unique_ptr<EvpHashCalculator> calc(new EvpHashCalculator(name));

    if (calc->mdctx_ == nullptr)
         return nullptr;

    return calc.release();
}
