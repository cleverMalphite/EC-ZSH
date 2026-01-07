#include <chrono>
#include <thread>
#include <future>
#include <functional>
#include <memory>
#include <thread_pool.hpp>

#include <vector>


int main(void){
    int task_num = 100;
    int inter_time = 1000;

    int pool_thrd_num = 100;
    int pool_queue_size = 32;

    printf("Create tasks ...");
    std::vector<std::packaged_task<int()>> tasks;
    for(int i = 0; i <task_num; i++){
        tasks.emplace_back([i, inter_time]()->int
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(inter_time));
                return i;
            });
    }
    printf("ok \n");


    printf("Create futures ...");
    std::vector<std::future<int>> rets;
    rets.reserve(task_num);

    for(int i = 0; i < task_num; i++){
        rets.emplace_back(tasks[i].get_future());
    }
    printf("ok\n");

    printf("Create Thread and set options ...");
    tp::ThreadPoolOptions options;
        options.setThreadCount(pool_thrd_num);
        options.setQueueSize(pool_queue_size);
    tp::ThreadPool pool(options);
    printf("ok\n");


    printf("Post tasks ...");
    for(int i = 0; i < task_num; i++){
        pool.post(tasks[i]);
    }
    printf("ok\n");

    printf("Get future ret : \n");
    for(int i = 0; i < task_num; i++){
        printf("get future ret of task 1 : %d\n", rets[i].get());
    }
    printf("End\n");

    return 0;
}
