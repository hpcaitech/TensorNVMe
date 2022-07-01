#include "../csrc/space_mgr.h"
#include <stdio.h>
#include <assert.h>

int main()
{
    SpaceManager space_mgr(0);
    ull offset = space_mgr.alloc(4);
    space_mgr.print();
    // [0, 4) is used
    assert(offset == 0);
    offset = space_mgr.alloc(8);
    space_mgr.print();
    // [0, 12) is used
    assert(offset == 4);
    space_mgr.free(4, 4);
    space_mgr.print();
    // [0, 4) and [8, 12) are used
    offset = space_mgr.alloc(2);
    space_mgr.print();
    // [0, 6) and [8, 12) are used
    assert(offset == 4);
    offset = space_mgr.alloc(4);
    space_mgr.print();
    // [0, 6) and [8, 16) are used
    assert(offset == 12);
    space_mgr.free(0, 2);
    space_mgr.print();
    space_mgr.free(4, 2);
    space_mgr.print();
    // [2, 4) and [8, 16) are used
    space_mgr.free(2, 2);
    space_mgr.print();
    // [8, 16) is used
    offset = space_mgr.alloc(5);
    space_mgr.print();
    // [0, 5) and [8, 16) are used
    assert(offset == 0);
    offset = space_mgr.alloc(4);
    space_mgr.print();
    // [0, 5) and [8, 20) are used
    assert(offset == 16);
    offset = space_mgr.alloc(2);
    space_mgr.print();
    // [0, 7) and [8, 20) are used
    assert(offset == 5);
}