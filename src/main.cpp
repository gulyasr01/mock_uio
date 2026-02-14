#include "../include/mock_uio.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <sys/eventfd.h>
#include <optional>
#include "../include/uiodevice.hpp"
#include "../include/mock_driver.hpp"

int main()
{
    const std::string dev_path = "./mock_uio.bin";

    printf("hello\n");

    //int uio_fd = create_mock_device(dev_path);

    int irq_fd = eventfd(0, EFD_CLOEXEC);
    if (irq_fd < 0)
    {
        perror("eventfd");
        return 1;
    }

    std::atomic<bool> cancel = false;

    std::jthread mock_device_thread([&]
                                   { mock_uio_dirver(dev_path, cancel, irq_fd); });

    // wait for the thread to create the mock device file
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    MockDriver driver{dev_path, kMapSize, irq_fd};
    std::optional<uint32_t> prev = std::nullopt;

    driver.star();
    printf("enabled\n");

    while (1)
    {
        uint32_t data = driver.wait_on_irq([&]() {
            std::atomic_thread_fence(std::memory_order_acquire);
            return driver.get_data();
        });

        if (prev.has_value()) {
            uint32_t prev_val = prev.value();

            if (data != (prev_val + 1)) {
                std::cout << "data loss - data is: " << data << " prev is: " << prev_val << std::endl;
                cancel.store(true, std::memory_order_relaxed);
                break;
            }
        }

        prev.emplace(data);
    }
    
    //mock_device_thread.join();

    // cleanup the mock device file
    std::remove(dev_path.c_str());

    return 0;
}