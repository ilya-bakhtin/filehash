#ifndef __HASH_CALCULATOR_H__
#define __HASH_CALCULATOR_H__

#include <vector>

#include <stddef.h>

class IHashCalculator
{
public:
    virtual ~IHashCalculator() {};

    virtual int init() = 0;
    virtual int update(const void* d, size_t cnt) = 0;
    virtual int final() = 0;

    virtual const std::vector<unsigned char>& get_hash() const = 0;

    static IHashCalculator* create(const char* name);

protected:
    IHashCalculator() {};
private:
    IHashCalculator(const IHashCalculator& other);
    IHashCalculator(IHashCalculator&& other);
    IHashCalculator& operator=(const IHashCalculator& other);
    IHashCalculator& operator=(IHashCalculator&& other);
};

#endif
