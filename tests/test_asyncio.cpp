#include "../csrc/aio.h"
#include <stdio.h>
#include <string.h>

int main()
{
    AsyncIO aio(2);
    int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    char *text = "TEST ME AGAIN!!!\n";
    size_t len = strlen(text);
    for (int i = 0; i < 10; i++)
    {
        aio.write(fd, text, len, i * len);
    }
    aio.sync_write_events();
    printf("done\n");
}
