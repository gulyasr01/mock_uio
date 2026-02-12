#include "../include/mock_uio.hpp"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <random>

static constexpr uint32_t kMagic = 0x55494F4D; // 'UIOM'
static constexpr uint32_t STATUS_IRQ_PENDING = 1u << 0;

struct __attribute__((packed)) Regs
{
    uint32_t MAGIC;     // 0x00
    uint32_t VERSION;   // 0x04
    uint8_t CTRL_1;   // 0x08
    uint8_t CTRL_2;   // 0x09
    uint16_t _pad_ctrl;
    uint32_t STATUS;    // 0x0C
    uint32_t IRQ_COUNT; // 0x10
    uint32_t IRQ_ACK;   // 0x14 (write-only in real hw; here we just observe writes)
    uint32_t DATA_IDX; // 0x18 index of the last falid data
    uint8_t _pad[MOCK_UIO_REG_DATA - MOCK_UIO_REG_DATA_IDX - 4]; // todo fix this logic: is it good with uintptr_t?
    uint32_t DATA[MOCK_UIO_REG_DATA_WLEN];
    uint8_t _pad_until_end[kMapSize - (MOCK_UIO_REG_DATA + MOCK_UIO_REG_DATA_WLEN *4)];
};

static_assert(offsetof(Regs, CTRL_1) == MOCK_UIO_REG_CTRL_1);
static_assert(offsetof(Regs, CTRL_2) == MOCK_UIO_REG_CTRL_2);
static_assert(offsetof(Regs, STATUS) == MOCK_UIO_REG_STATUS);
static_assert(offsetof(Regs, IRQ_ACK) == MOCK_UIO_REG_IRQ_ACK);
static_assert(offsetof(Regs, DATA) == MOCK_UIO_REG_DATA);
static_assert(sizeof(Regs) == kMapSize);

// exported functions
int create_register_file(const std::string &path)
{
    int fd = ::open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    if (ftruncate(fd, kMapSize) != 0)
    {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    // Initialize content via a temporary map
    void *p = mmap(nullptr, kMapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return -1;
    }

    auto *r = reinterpret_cast<Regs *>(p);
    std::memset(r, 0, sizeof(*r));
    r->MAGIC = kMagic;
    r->VERSION = 1;
    r->CTRL_1 = 2;
    r->CTRL_2 = 3;
    r->STATUS = 0;
    r->IRQ_COUNT = 0;
    r->IRQ_ACK = 0;
    r->DATA[0] = 0x12345678;

    if (msync(p, kMapSize, MS_SYNC) != 0)
        perror("msync");
    munmap(p, kMapSize);

    printf("file ready\n");
    return fd;
}

int mock_uio_dirver(const std::string &path, std::atomic<bool> &cancel, int irq_fd) {
    int fd = ::open(path.c_str(), O_RDWR, 0644);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    if (ftruncate(fd, kMapSize) != 0)
    {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    // Initialize content via a temporary map
    void *p = mmap(nullptr, kMapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return -1;
    }

    auto *regs = reinterpret_cast<Regs *>(p);

    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dist(0, MOCK_UIO_REG_DATA_WLEN -1);

    uint32_t counter = 0;

    while (!cancel.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        int random_idx = dist(gen) % MOCK_UIO_REG_DATA_WLEN; 

        std::cout << "written " << counter << " to " << random_idx << std::endl;

        // "hardware" writes
        regs->DATA[random_idx] = counter++;
        regs->DATA_IDX = random_idx;
        regs->STATUS |= STATUS_IRQ_PENDING;
        regs->IRQ_COUNT += 1;

        // "interrupt line" -> userspace wakeup
        uint64_t one = 1;
        ssize_t wr = ::write(irq_fd, &one, sizeof(one));
        if (wr != (ssize_t)sizeof(one)) {
            perror("write(eventfd)");
            break;
        }
    }

    return 0;
}