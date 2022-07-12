#define CATCH_CONFIG_MAIN

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <functional>
#include <unistd.h>
#include "aio.h"
#include "catch.hpp"
#include "uring.h"
#include "offload.h"
using namespace std;


TEST_CASE( "Test offload" ) {
    Offloader *offload = new Offloader("./test", 2, "uring");
    at::Tensor tensor1 = at::rand({12, 12}), tensor2 = at::rand({12, 12}), tensor3 = at::rand({12, 12});
    offload->async_write(tensor1, "12345");
    offload->sync_write_events();
    offload->async_read(tensor2, "12345");
    offload->sync_write_events();

    auto foo_1 = tensor1.accessor<float, 2>();
    auto foo_2 = tensor2.accessor<float, 2>();
    auto foo_3 = tensor3.accessor<float, 2>();

    for(int i = 0; i < 12; i++) {
        for(int j = 0; j < 12; j++) {
            // use the accessor foo_a to get tensor data.
            REQUIRE(foo_1[i][j] == foo_2[i][j]);
            REQUIRE(foo_1[i][j] != foo_3[i][j]);
        }
    }
}
