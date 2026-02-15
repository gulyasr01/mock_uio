#pragma once

// #include "mock_uio.hpp"

#include <string>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>

class unique_fd {
private:
    int fd{-1};
public:
    unique_fd() = default;

    explicit unique_fd(const std::string dev_node) {
        fd = open(dev_node.c_str(), O_RDWR, 0644);
        if (fd < 0)
        {
            throw std::runtime_error("File cannot be opened: " + dev_node);
        }
    }

    ~unique_fd() {
        if (fd >= 0 ) {
            close(fd);
        }
    }

    // copy
    unique_fd(const unique_fd& other) = delete;
    
    unique_fd& operator=(const unique_fd& other) = delete;
    
    // move
    unique_fd& operator=(unique_fd&& other) noexcept {
        if (this != &other) {
            if (fd >=0) close(fd);
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }

    unique_fd(unique_fd && other) noexcept : fd(other.fd) {other.fd = -1;}

    // getter
    int get() noexcept {return fd;}
};

class unique_mmap {
private:
    void * p{MAP_FAILED};
    size_t len{0};

public:
    unique_mmap() = default;

    unique_mmap(int fd, size_t size) : len(size) {
        p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        
        if (p == MAP_FAILED)
        {
            throw std::runtime_error("Cannot mmap fd : " + std::to_string(fd));
        }
    }

    unique_mmap& operator=(const unique_mmap & other) = delete;
    unique_mmap(const unique_mmap & other) = delete;

    unique_mmap(unique_mmap && other) : p(other.p), len(other.len) {other.p = MAP_FAILED; other.len = 0;}

    unique_mmap& operator=(unique_mmap && other) noexcept {
        if (this != &other) {
            if (p != MAP_FAILED) {
                munmap(p, len);
                p = other.p;
                len = other.len;
                other.p = MAP_FAILED;
                other.len = 0;
            }
        }
        return *this;
    }

    std::byte * get() const noexcept{return static_cast<std::byte*>(p);}
};

class UioDevice
{
public:
    UioDevice(const std::string devnode, size_t devsize, int irqfd) : 
        dev_node(std::move(devnode)), 
        dev_size(devsize), 
        irq_fd(irqfd),
        fd(dev_node),
        base(fd.get(), dev_size)
    {}

    UioDevice(UioDevice &&other) = delete;
    UioDevice(const UioDevice &other) = delete;
    UioDevice &operator=(const UioDevice &other) = delete;
    UioDevice &operator=(UioDevice &&other) = delete;

    ~UioDevice() = default;

    uint32_t read32(std::uintptr_t offs)
    {
        bounds_check(offs, sizeof(uint32_t));
        uint32_t data = *reinterpret_cast<volatile uint32_t *>(base.get() + offs);
        return data;
    }

    void write32(std::uintptr_t offs, uint32_t val)
    {
        bounds_check(offs, sizeof(uint32_t));
        *reinterpret_cast<volatile uint32_t *>(base.get() + offs) = val;
    }

    uint64_t wait_for_irq()
    {
        uint64_t cnt;
        ssize_t n = read(irq_fd, &cnt, sizeof(cnt));
        if (n != sizeof(cnt))
            throw std::runtime_error("read failed");
        return cnt;
    }

private:
    void bounds_check(std::uintptr_t offs, size_t sz)
    {
        if (offs + sz > dev_size)
        {
            throw std::runtime_error("Offset with size is out of bound of devsize:");
        }
    }

    std::string dev_node;
    size_t dev_size;
    int irq_fd;
    unique_fd fd;
    unique_mmap base;
};
