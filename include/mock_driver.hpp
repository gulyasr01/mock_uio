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
        return driver.read32(data_addr);
    }

    void irq_handler() {
        driver.wait_for_irq();
        uint32_t data = get_data();
        std::cout << "received data: " << data << std::endl;
    }

private:
    UioDevice driver;
};
