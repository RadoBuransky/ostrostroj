#pragma once

#include <vector>
#include "project.hpp"

class Workspace {
    private:
        const std::filesystem::path dir;
        std::vector<Project2> projects;
        // TODO: Load list of projects
    public:
        Workspace(const std::filesystem::path dir);
        virtual ~Workspace();
        std::vector<Project2>& get_projects();
};