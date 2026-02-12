#include "mock_uio.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <sys/eventfd.h>

int main()
{
    const std::string dev_path = "./mock_uio.bin";

    printf("hello\n");

    int uio_fd = create_register_file(dev_path);

    printf("we got the file");

    int irq_fd = eventfd(0, EFD_CLOEXEC);
    if (irq_fd < 0)
    {
        perror("eventfd");
        return 1;
    }

    printf("we got the intr");

    std::atomic<bool> cancel = false;

    std::jthread mock_device_thread([&]
                                   { mock_uio_dirver(dev_path, cancel, irq_fd); });

    //mock_device_thread.join();

    return 0;
}