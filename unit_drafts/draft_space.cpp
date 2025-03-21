#include "space.h"

int main()
{
    Space space;
    space.resize(8, 8);
    space.place(1, 0, 1);
    space.place(2, 1, 1);
    space.place(3, 2, 1);
    space.print();
    auto a = space.check_win();
}
