#include <unistd.h>

int setproctitle(void** pp) {return 0;}
int main() {
    void* p;
    return setproctitle(&p);
}

