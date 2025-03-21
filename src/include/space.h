#ifndef SPACE_H
#define SPACE_H

#include <vector>

class Space
{
private:
    uint64_t width{}, height{};
    std::vector<std::vector<signed char>> desk;
    signed win_within_3x3(int x, int y);

public:
    // resize table to a new size (only larger table size is accepted)
    void resize(int new_width, int new_height);
    Space();

    // place an object in the map. 0 for X and 1 for O
    void place(int x, int y, signed char c);

    // get the specific object, 0 for X, 1 for O, and -1 for empty
    [[nodiscard]] signed char get(int x, int y) const;

    // print out current table
    void print() const;

    // check if anyone is winning. 0 for X winning, 1 for O winning, -1 for none.
    signed check_win();
};

#endif //SPACE_H
