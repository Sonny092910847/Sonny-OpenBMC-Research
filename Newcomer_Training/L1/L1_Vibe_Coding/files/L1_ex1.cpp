#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    while (true)
    {
        std::cout << "BMC L1 Training - ID: 835051, Name: Sonny Chu" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}
