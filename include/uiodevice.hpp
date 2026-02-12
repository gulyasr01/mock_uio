#pragma once

// #include "mock_uio.hpp"

#include <string>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>

class UioDevice
{
public:
    UioDevice(const std::string devnode, size_t devsize, int irqfd) : dev_node(std::move(devnode)), dev_size(devsize), irq_fd(irqfd)
    {
        fd = open(dev_node.c_str(), O_RDWR, 0644);
        if (fd < 0)
        {
            throw std::runtime_error("File cannot be opened: " + dev_node);
        }

        void *p = mmap(nullptr, devsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
        {
            throw std::runtime_error("Cannot mmap : " + dev_node);
        }

        base = static_cast<std::byte *>(p);
    }

    UioDevice(UioDevice &&other) = delete;
    UioDevice(const UioDevice &other) = delete;
    UioDevice &operator=(const UioDevice &other) = delete;
    UioDevice &operator=(UioDevice &&other) = delete;

    ~UioDevice()
    {
        if (base != nullptr)
        {
            munmap(base, dev_size);
        }
        if (fd >= 0)
        {
            close(fd);
        }
    }

    uint32_t read32(std::uintptr_t offs)
    {
        bounds_check(offs, sizeof(uint32_t));
        return *reinterpret_cast<volatile uint32_t *>(base + offs);
    }

    void write32(std::uintptr_t offs, uint32_t val)
    {
        bounds_check(offs, sizeof(uint32_t));
        *reinterpret_cast<volatile uint32_t *>(base + offs) = val;
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

    int fd;
    int irq_fd;
    size_t dev_size;
    std::byte *base = nullptr;
    std::string dev_node;
};
