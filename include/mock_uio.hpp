#pragma once

#include <string>
#include <atomic>
#include <cstdint>

constexpr size_t kMapSize = 4096;

constexpr std::uintptr_t MOCK_UIO_REG_MAGIC     = 0x00;
constexpr std::uintptr_t MOCK_UIO_REG_VERSION   = 0x04;
constexpr std::uintptr_t MOCK_UIO_REG_CTRL_1    = 0x08;
constexpr std::uintptr_t MOCK_UIO_REG_CTRL_2    = 0x09;
constexpr std::uintptr_t MOCK_UIO_REG_STATUS    = 0x0C;
constexpr std::uintptr_t MOCK_UIO_REG_IRQ_COUNT = 0x10;
constexpr std::uintptr_t MOCK_UIO_REG_IRQ_ACK   = 0x14;
constexpr std::uintptr_t MOCK_UIO_REG_DATA_IDX  = 0x18;
constexpr std::uintptr_t MOCK_UIO_REG_DATA      = 0x100; // 256 words

constexpr std::uint32_t MOCK_UIO_REG_DATA_WLEN = 256; // 256 words

constexpr uint8_t MOCK_UIO_STATUS_WARMUP = 0;
constexpr uint8_t MOCK_UIO_STATUS_IO_ERR = 1;
constexpr uint8_t MOCK_UIO_STATUS_TMP_ERR = 2;
constexpr uint8_t MOCK_UIO_STATUS_DATA_RDY = 8;

int create_register_file(const std::string &path);
int mock_uio_dirver(const std::string &path, std::atomic<bool> &cancel, int irq_fd);