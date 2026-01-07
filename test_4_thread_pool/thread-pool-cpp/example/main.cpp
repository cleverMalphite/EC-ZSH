#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include <memory>
#include <thread_pool.hpp>


int main(void){


    std::packaged_task<int()> task1([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 1;
        });

    std::packaged_task<int()> task2([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 2;
        });

    std::packaged_task<int()> task3([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 3;
        });


    std::future<int> ret1 = task1.get_future();
    std::future<int> ret2 = task2.get_future();
    std::future<int> ret3 = task3.get_future();


    tp::ThreadPool pool;
    pool.post(task1);
    pool.post(task2);
    pool.post(task3);

    printf("get future ret of task 1 : %d\n", ret1.get());
    printf("get future ret of task 2 : %d\n", ret2.get());
    printf("get future ret of task 3 : %d\n", ret3.get());

    return 0;
}
