#include <stdio.h>
#include <string.h>
#include <functional>
#include <unistd.h>
#include "../csrc/uring.h"

int n = 0;

void callback_n(int &x)
{
    printf("Write %d\n", x++);
}

int main()
{
    AsyncIO *aio = new UringAsyncIO(2);
    int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    char *text = "TEST ME AGAIN!!!\n";
    size_t len = strlen(text);
    for (int i = 0; i < 10; i++)
    {
        auto fn = std::bind(callback_n, std::ref(n));
        aio->write(fd, text, len, i * len, fn);
    }
    aio->sync_write_events();
    delete aio;
    close(fd);
    remove("./test.txt");
    printf("done\n");
}
