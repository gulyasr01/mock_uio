#include "../include/mock_uio.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <sys/eventfd.h>
#include "../include/uiodevice.hpp"
#include "../include/mock_driver.hpp"

int main()
{
    const std::string dev_path = "./mock_uio.bin";

    printf("hello\n");

    int uio_fd = create_register_file(dev_path);

    int irq_fd = eventfd(0, EFD_CLOEXEC);
    if (irq_fd < 0)
    {
        perror("eventfd");
        return 1;
    }

    MockDriver driver{dev_path, kMapSize, irq_fd};

    std::atomic<bool> cancel = false;

    std::jthread mock_device_thread([&]
                                   { mock_uio_dirver(dev_path, cancel, irq_fd); });

    driver.star();

    while (1)
    {
        driver.wait_on_irq();
        driver.get_data();
        driver.ack_irq();
    }
    

    //mock_device_thread.join();

    return 0;
}