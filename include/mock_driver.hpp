#pragma once

#include "mock_uio.hpp"
#include "uiodevice.hpp"

#include <iostream>

class MockDriver {
public:
    MockDriver(std::string devnode, size_t devsize, int irq_fd): driver(std::move(devnode), devsize, irq_fd) {}

    uint32_t get_data() {
        uint32_t data_idx = driver.read32(MOCK_UIO_REG_DATA_IDX);
        if (data_idx >= MOCK_UIO_REG_DATA_WLEN) {
            throw std::runtime_error("data idx out of bound - broken device");
        }
        uintptr_t data_addr = MOCK_UIO_REG_DATA + data_idx * sizeof(uint32_t);
        uint32_t data = driver.read32(data_addr);
        std::cout << "received " << data << " on " << data_addr << std::endl;

        return data;
    }

    void wait_on_irq() {
        driver.wait_for_irq();
    }

    void ack_irq() {
        driver.write32(MOCK_UIO_REG_IRQ_ACK, u_int32_t{1});
    }

    void star() {
        uint32_t data_ctrl = driver.read32(MOCK_UIO_REG_CTRL_1);
        uint32_t mask = uint32_t{1} << 8;
        data_ctrl |= mask;
        driver.write32(MOCK_UIO_REG_CTRL_1, data_ctrl);
    }

private:
    UioDevice driver;
};
