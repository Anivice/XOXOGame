#ifndef SPACE_H
#define SPACE_H

#include <vector>

class Space
{
private:
    uint64_t width{}, height{};
    std::vector<std::vector<signed char>> desk;


public:
    void resize(int new_width, int new_height);
    Space();
    void place(int x, int y, signed char c);
    [[nodiscard]] signed char get(int x, int y) const;
    void print() const;
    signed win_within_3x3(int x, int y);
    signed check_win();
};

#endif //SPACE_H
