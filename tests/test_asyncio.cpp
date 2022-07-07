#define CATCH_CONFIG_MAIN

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <functional>
#include <unistd.h>
#include "aio.h"
#include "catch.hpp"
#include "uring.h"
using namespace std;

void callback_n(int &x)
{
    x++;
}

void callback_empty()
{
}

TEST_CASE( "Test async io fucntion of libaio and liburing") {

    AsyncIO *aios[] = {
            new AIOAsyncIO(1),
            new AIOAsyncIO(10),
            new AIOAsyncIO(100),
            new AIOAsyncIO(1000),
            new UringAsyncIO(1),
            new UringAsyncIO(10),
            new UringAsyncIO(100),
            new UringAsyncIO(1000),
    };

    for(int aio_idx = 0; aio_idx < sizeof(aios) / sizeof(aios[0]); aio_idx++){
        auto aio = aios[aio_idx];

        SECTION("write char array to a file") {
            int fd = open("./tmp_test", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            char text[5][18] = {"TEST ME AGAIN!!!\n", "TEST ME AGAIN!!!\n", "TEST ME AGAIN!!!\n", "TEST ME AGAIN!!!\n",
                                "TEST ME AGAIN!!!\n"};
            size_t len = strlen(text[0]) + 1;
            int n = 0;
            for (int i = 0; i < 5; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                aio->write(fd, text[i], len, i * len, fn);
            }
            aio->sync_write_events();
            REQUIRE(n == 5);

            char new_text[5][18];
            read(fd, new_text, 5 * 18 * sizeof(char));
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 18; j++) {
                    REQUIRE(text[i][j] == new_text[i][j]);
                }
            }
            close(fd);
            remove("./tmp_test");
        }

        SECTION("read char array from a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            char text[5][18] = {"TEST ME AGAIN!!!\n", "TEST ME AGAIN!!!\n", "TEST ME AGAIN!!!\n", "TEST ME AGAIN!!!\n",
                                "TEST ME AGAIN!!!\n"};
            write(fd, text, 5 * 18 * sizeof(char));

            char new_text[5][18];
            size_t len = strlen(text[0]) + 1;
            int n = 0;
            for (int i = 0; i < 5; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                aio->read(fd, new_text[i], len, i * len, fn);
            }
            aio->sync_read_events();
            REQUIRE(n == 5);
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 18; j++) {
                    REQUIRE(text[i][j] == new_text[i][j]);
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write char array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 5, n_len = 18;

            char text[n_loop][n_len];

            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(text[i]) + 1;
                len = n_len;
                aio->write(fd, text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            char new_text[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len;
                aio->read(fd, new_text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(text[i][j] == new_text[i][j]);
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write none char array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            const int n_loop = 0, n_len = 0;

            char text[n_loop][n_len];

            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(text[i]) + 1;
                len = n_len;
                aio->write(fd, text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            char new_text[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len;
                aio->read(fd, new_text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(text[i][j] == new_text[i][j]);
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write small char array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 1, n_len = 1;

            char text[n_loop][n_len];

            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(text[i]) + 1;
                len = n_len;
                aio->write(fd, text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();
            REQUIRE(n == n_loop);

            char new_text[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len;
                aio->read(fd, new_text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            REQUIRE(n == n_loop);
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(text[i][j] == new_text[i][j]);
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write large char array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 1000, n_len = 1000;

            char text[n_loop][n_len];

            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(text[i]) + 1;
                len = n_len;
                aio->write(fd, text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            char new_text[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len;
                aio->read(fd, new_text[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(text[i][j] == new_text[i][j]);
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write double array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 5, n_len = 18;

            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(data1[i]) + 1;
                len = n_len * sizeof(double);
                aio->write(fd, data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fd, data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write none double array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 0, n_len = 0;

            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(data1[i]) + 1;
                len = n_len * sizeof(double);
                aio->write(fd, data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fd, data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write small double array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 1, n_len = 1;

            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(data1[i]) + 1;
                len = n_len * sizeof(double);
                aio->write(fd, data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fd, data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write large double array to a file") {
            int fd = open("./test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            const int n_loop = 100, n_len = 1000;

            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                //len = strlen(data1[i]) + 1;
                len = n_len * sizeof(double);
                aio->write(fd, data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fd, data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }
            close(fd);
            remove("./test.txt");
        }

        SECTION("read and write double array to multiple files") {
            const int n_loop = 5, n_len = 18;
            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            int fds[n_loop];
            char file_name[] = "testn";

            for (int i = 0; i < n_loop; i++) {
                file_name[4] = (char) (i + 1);
                fds[i] = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->write(fds[i], data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();
            REQUIRE(n == n_loop);

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fds[i], data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            REQUIRE(n == n_loop);

            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }

            for (int i = 0; i < n_loop; i++) {
                close(fds[i]);
                file_name[4] = (char) (i + 1);
                remove(file_name);
            }
        }

        SECTION("read and write none double array to multiple files") {
            const int n_loop = 10, n_len = 0;
            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            int fds[n_loop];
            char file_name[] = "testn";

            for (int i = 0; i < n_loop; i++) {
                file_name[4] = (char) (i + 1);
                fds[i] = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->write(fds[i], data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fds[i], data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();

            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }

            for (int i = 0; i < n_loop; i++) {
                close(fds[i]);
                file_name[4] = (char) (i + 1);
                remove(file_name);
            }
        }

        SECTION("read and write small double array to multiple files") {
            const int n_loop = 1, n_len = 1;
            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            int fds[n_loop];
            char file_name[] = "testn";

            for (int i = 0; i < n_loop; i++) {
                file_name[4] = (char) (i + 1);
                fds[i] = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->write(fds[i], data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fds[i], data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();

            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }

            for (int i = 0; i < n_loop; i++) {
                close(fds[i]);
                file_name[4] = (char) (i + 1);
                remove(file_name);
            }
        }

        SECTION("read and write large double array to multiple files") {
            const int n_loop = 500, n_len = 500;
            double data1[n_loop][n_len];
            int n = 0, offset = 0;
            size_t len;
            int fds[n_loop];
            string filename = "test", new_name;

            for (int i = 0; i < n_loop; i++) {
                new_name = filename + to_string(i);
                fds[i] = open(new_name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->write(fds[i], data1[i], len, offset, fn);
                offset += len;
            }
            aio->sync_write_events();
            REQUIRE(n == n_loop);

            double data2[n_loop][n_len];
            n = 0, offset = 0;
            for (int i = 0; i < n_loop; i++) {
                auto fn = std::bind(callback_n, std::ref(n));
                len = n_len * sizeof(double);
                aio->read(fds[i], data2[i], len, offset, fn);
                offset += len;
            }
            aio->sync_read_events();
            REQUIRE(n == n_loop);

            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }

            for (int i = 0; i < n_loop; i++) {
                close(fds[i]);
                new_name = filename + to_string(i);
                remove(new_name.c_str());
            }
        }

        SECTION("use nullptr cb to read and write double array to multiple files") {
            const int n_loop = 5, n_len = 18;
            double data1[n_loop][n_len];
            int offset = 0;
            size_t len;
            int fds[n_loop];
            char file_name[] = "testn";

            for (int i = 0; i < n_loop; i++) {
                file_name[4] = (char) (i + 1);
                fds[i] = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                len = n_len * sizeof(double);
                REQUIRE_NOTHROW(aio->write(fds[i], data1[i], len, offset, nullptr));
                offset += len;
            }
            REQUIRE_NOTHROW(aio->sync_write_events());

            double data2[n_loop][n_len];
            offset = 0;
            for (int i = 0; i < n_loop; i++) {
                len = n_len * sizeof(double);
                REQUIRE_NOTHROW(aio->read(fds[i], data2[i], len, offset, nullptr));
                offset += len;
            }
            REQUIRE_NOTHROW(aio->sync_read_events());

            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }

            for (int i = 0; i < n_loop; i++) {
                close(fds[i]);
                file_name[4] = (char) (i + 1);
                remove(file_name);
            }
        }

        SECTION("use empty cb to read and write double array to multiple files") {
            const int n_loop = 5, n_len = 18;
            double data1[n_loop][n_len];
            int offset = 0;
            size_t len;
            int fds[n_loop];
            char file_name[] = "testn";

            for (int i = 0; i < n_loop; i++) {
                file_name[4] = (char) (i + 1);
                fds[i] = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                len = n_len * sizeof(double);
                REQUIRE_NOTHROW(aio->write(fds[i], data1[i], len, offset, callback_empty));
                offset += len;
            }
            REQUIRE_NOTHROW(aio->sync_write_events());

            double data2[n_loop][n_len];
            offset = 0;
            for (int i = 0; i < n_loop; i++) {
                len = n_len * sizeof(double);
                REQUIRE_NOTHROW(aio->read(fds[i], data2[i], len, offset, callback_empty));
                offset += len;
            }
            REQUIRE_NOTHROW(aio->sync_read_events());

            for (int i = 0; i < n_loop; i++) {
                for (int j = 0; j < n_len; j++) {
                    REQUIRE(data1[i][j] == Approx(data2[i][j]).epsilon(0.001));
                }
            }

            for (int i = 0; i < n_loop; i++) {
                close(fds[i]);
                file_name[4] = (char) (i + 1);
                remove(file_name);
            }
        }

        delete aio;
    }
}
