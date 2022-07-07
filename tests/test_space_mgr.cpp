#define CATCH_CONFIG_MAIN
#include <stdio.h>
#include "catch.hpp"

#define private public
#include "space_mgr.h"


TEST_CASE( "Test space manager function" ) {
    SpaceManager space_mgr(0);
    SpaceInfo new_avail_space;
    vector<SpaceInfo> test_avail_spaces;
    ull offset;

    SECTION( "origin test" ){
        offset = space_mgr.alloc(4);
        // [0, 4) is used
        REQUIRE(offset == 0);
        offset = space_mgr.alloc(8);
        // [0, 12) is used
        REQUIRE(offset == 4);
        space_mgr.free(4, 4);
        // [0, 4) and [8, 12) are used
        offset = space_mgr.alloc(2);
        // [0, 6) and [8, 12) are used
        REQUIRE(offset == 4);
        offset = space_mgr.alloc(4);
        // [0, 6) and [8, 16) are used
        REQUIRE(offset == 12);
        space_mgr.free(0, 2);
        space_mgr.free(4, 2);
        // [2, 4) and [8, 16) are used
        space_mgr.free(2, 2);
        // [8, 16) is used
        offset = space_mgr.alloc(5);
        // [0, 5) and [8, 16) are used
        REQUIRE(offset == 0);
        offset = space_mgr.alloc(4);
        // [0, 5) and [8, 20) are used
        REQUIRE(offset == 16);
        offset = space_mgr.alloc(2);
        // [0, 7) and [8, 20) are used
        REQUIRE(offset == 5);
    }

    SECTION( "alloc and" ) {
        offset = space_mgr.alloc(20);
        REQUIRE(offset == 0);
        REQUIRE(space_mgr.used_bytes == 20);
        auto test_iter = test_avail_spaces.begin();
        for (auto iter = space_mgr.avail_spaces.begin(); iter != space_mgr.avail_spaces.end(); iter++){
            REQUIRE(iter->first == test_iter->first);
            REQUIRE(iter->second == test_iter->second);
            REQUIRE_NOTHROW(test_iter++);
        }

        SECTION( "drop the last one and" ) {
            space_mgr.free(19, 1);
            new_avail_space.first = 19;
            new_avail_space.second = 1;
            test_avail_spaces.push_back(new_avail_space);
            test_iter = test_avail_spaces.begin();
            for (auto iter = space_mgr.avail_spaces.begin(); iter != space_mgr.avail_spaces.end(); iter++) {
                REQUIRE(iter->first == test_iter->first);
                REQUIRE(iter->second == test_iter->second);
                REQUIRE_NOTHROW(test_iter++);
            }
            REQUIRE(space_mgr.used_bytes == 19);

            SECTION( "alloc a little one" ) {
                offset = space_mgr.alloc(1);
                REQUIRE(offset == 19);
                REQUIRE(space_mgr.used_bytes == 20);
            }

            SECTION( "alloc a big one" ) {
                offset = space_mgr.alloc(2);
                CHECK(offset == 19);
                CHECK(space_mgr.used_bytes == 21);
            }

            SECTION( "alloc a bigger one" ) {
                offset = space_mgr.alloc(10);
                CHECK(offset == 19);
                CHECK(space_mgr.used_bytes == 29);
            }
        }

        SECTION( "drop the last ones and" ) {
            space_mgr.free(16, 4);
            new_avail_space.first = 16;
            new_avail_space.second = 4;
            test_avail_spaces.push_back(new_avail_space);
            test_iter = test_avail_spaces.begin();
            for (auto iter = space_mgr.avail_spaces.begin(); iter != space_mgr.avail_spaces.end(); iter++) {
                REQUIRE(iter->first == test_iter->first);
                REQUIRE(iter->second == test_iter->second);
                REQUIRE_NOTHROW(test_iter++);
            }
            REQUIRE(space_mgr.used_bytes == 16);

            SECTION( "alloc a little one" ) {
                offset = space_mgr.alloc(4);
                REQUIRE(offset == 16);
                REQUIRE(space_mgr.used_bytes == 20);
            }

            SECTION( "alloc a big one" ) {
                offset = space_mgr.alloc(5);
                CHECK(offset == 16);
                CHECK(space_mgr.used_bytes == 21);
            }

            SECTION( "alloc a bigger one" ) {
                offset = space_mgr.alloc(10);
                CHECK(offset == 16);
                CHECK(space_mgr.used_bytes == 26);
            }
        }

        SECTION( "drop middle ones and" ) {
            space_mgr.free(10, 4);
            new_avail_space.first = 10;
            new_avail_space.second = 4;
            test_avail_spaces.push_back(new_avail_space);
            test_iter = test_avail_spaces.begin();
            for (auto iter = space_mgr.avail_spaces.begin(); iter != space_mgr.avail_spaces.end(); iter++) {
                REQUIRE(iter->first == test_iter->first);
                REQUIRE(iter->second == test_iter->second);
                REQUIRE_NOTHROW(test_iter++);
            }
            REQUIRE(space_mgr.used_bytes == 20);

            SECTION( "alloc a small one and" ) {
                offset = space_mgr.alloc(4);
                REQUIRE(offset == 10);
                REQUIRE(space_mgr.used_bytes == 20);
            }

            SECTION( "alloc a big one and" ) {
                offset = space_mgr.alloc(5);
                REQUIRE(offset == 20);
                REQUIRE(space_mgr.used_bytes == 25);
            }
        }

        SECTION( "drop the first one and" ) {
            space_mgr.free(0, 1);
            new_avail_space.first = 0;
            new_avail_space.second = 1;
            test_avail_spaces.push_back(new_avail_space);
            test_iter = test_avail_spaces.begin();
            for (auto iter = space_mgr.avail_spaces.begin(); iter != space_mgr.avail_spaces.end(); iter++) {
                REQUIRE(iter->first == test_iter->first);
                REQUIRE(iter->second == test_iter->second);
                REQUIRE_NOTHROW(test_iter++);
            }
            REQUIRE(space_mgr.used_bytes == 20);

            SECTION( "alloc a small one and" ) {
                offset = space_mgr.alloc(1);
                REQUIRE(offset == 0);
                REQUIRE(space_mgr.used_bytes == 20);
            }

            SECTION( "alloc a big one and" ) {
                offset = space_mgr.alloc(2);
                REQUIRE(offset == 20);
                REQUIRE(space_mgr.used_bytes == 22);
            }
        }
    }

}