#pragma once

#include <vector>
#include "sample.hpp"

struct Program {
    private:
        std::vector<Sample> loops;
        std::vector<Sample> shots;
};

class Project {
    private:
        std::vector<Program> programs;
};
