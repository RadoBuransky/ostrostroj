#pragma once

#include <vector>
#include "project.hpp"

class Workspace {
    private:
        const std::filesystem::path dir;
        std::vector<Project2> projects;
    public:
        Workspace(const std::filesystem::path _dir);
        virtual ~Workspace() = default;
        std::vector<Project2>& get_projects();
};