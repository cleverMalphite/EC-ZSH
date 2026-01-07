#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>

#include "rpc/server.h"
#include "rpc/this_handler.h"

using namespace std;

double divide(double a, double b) {
    return a / b;
}
int main() {
    rpc::server srv(rpc::constants::DEFAULT_PORT);
    // ... free functions                                                                                                   
    srv.bind("div", &divide);
    srv.run();
	return 0;
}
