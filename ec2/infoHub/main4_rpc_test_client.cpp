#include <unistd.h>
#include "rpc/client.h"

#include <iostream>

int main() {
    rpc::client c("localhost", rpc::constants::DEFAULT_PORT);

    printf("connection state : %d\n", c.get_connection_state());

    std::string text;
    while (1) {
        double result = c.call("div", 4, 2).as<double>();
        std::cout << "> " <<  result << std::endl;

        sleep(1);
    }
}
