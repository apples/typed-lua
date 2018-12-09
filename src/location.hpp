#pragma once

#include <iostream>

namespace typedlua {

struct Location {
    int first_line = 0;
    int first_column = 0;
    int last_line = 0;
    int last_column = 0;
};

} // namespace typedlua
