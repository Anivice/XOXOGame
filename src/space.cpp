#include "space.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>    // for std::fill and std::swap
#include <cstring>      // for std::memcpy

void Space::resize(int new_width, int new_height)
{
    if (new_height < 3 || new_width < 3) {
        throw std::invalid_argument("Invalid size");
    }

    // Swap new_width and new_height using std::swap instead of arithmetic swapping.
    std::swap(new_width, new_height);

    // Resize the desk: new_width becomes the number of rows, new_height the number of columns.
    desk.resize(new_width);
    for (auto & row : desk) {
        row.resize(new_height);
    }

    // If the previous board had fewer columns, fill the new parts with -1.
    if (width && width < new_width) {
        for (auto & row : desk) {
            // Start filling at index "width" (instead of width-1) for clarity.
            std::fill(row.begin() + static_cast<size_t>(width), row.end(), -1);
        }
    }

    // If the previous board had fewer rows, fill the new rows with -1.
    if (height && height < new_height) {
        for (auto it = desk.begin() + static_cast<size_t>(height); it < desk.end(); ++it) {
            std::ranges::fill(*it, -1);
        }
    }

    // Swap back so that new_width becomes width and new_height becomes height.
    std::swap(new_width, new_height);
    width = new_width;
    height = new_height;
}

Space::Space()
{
    resize(3, 3);
    for (auto & row : desk) {
        for (auto & point : row) {
            point = -1;
        }
    }
}

void Space::place(const int x, const int y, const signed char c)
{
    if (x >= 0 && x < width && y >= 0 && y < height) {
        desk[y][x] = c;
    } else {
        throw std::out_of_range("Placement out of range");
    }
}

signed char Space::get(const int x, const int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return desk[y][x];
    } else {
        throw std::out_of_range("Index out of range");
    }
}

void Space::print() const
{
    std::stringstream output;
    output << std::string(width + 2, '+') << std::endl;
    for (auto & row : desk)
    {
        output << "+";
        for (auto & point : row)
        {
            if (point == -1) {
                output << "-";
                continue;
            }
            output << (point == 0 ? 'X' : 'O');
        }
        output << "+\n";
    }
    output << std::string(width + 2, '+') << std::endl;
    std::cout << output.str() << std::flush;
}

#define CHECK(x1, y1, x2, y2, x3, y3)                                             \
    if (const auto dat = is_same(array[x1][y1], array[x2][y2], array[x3][y3]);    \
        dat != -1) {                                                              \
        return dat;                                                               \
    }

signed Space::win_within_3x3(const int x, const int y)
{
    char array[3][3];
    // Precompute starting iterator to avoid repeated calculations.
    auto row_start = desk.begin() + y;
    for (int i = 0; i < 3; ++i, ++row_start) {
        std::memcpy(array[i], row_start->data() + x, 3);
    }

    auto is_same = [&](const char data1, const char data2, const char data3) -> signed
    {
        if (data1 == data2 && data3 == data1) {
            return data1;
        }
        return -1;
    };

    CHECK(0, 0, 1, 0, 2, 0)
    CHECK(0, 1, 1, 1, 2, 1)
    CHECK(0, 2, 1, 2, 2, 2)
    CHECK(0, 0, 0, 1, 0, 2)
    CHECK(1, 0, 1, 1, 1, 2)
    CHECK(2, 0, 2, 1, 2, 2)
    CHECK(0, 0, 1, 1, 2, 2)
    CHECK(0, 2, 1, 1, 2, 0)

    return -1;
}

signed Space::check_win()
{
    if (width == 3 && height == 3) {
        return win_within_3x3(0, 0);
    }

    // Precompute the maximum starting positions.
    const int max_x = width - 3;
    const int max_y = height - 3;
    for (int x = 0; x < max_x; x++) {
        for (int y = 0; y < max_y; y++) {
            if (const auto dat = win_within_3x3(x, y); dat != -1)
            {
                return dat;
            }
        }
    }
    return -1;
}
