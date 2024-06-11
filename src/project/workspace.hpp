#pragma once

#include "project.hpp"

class Workspace {
    private:
        std::vector<Project> projects;
        std::vector<Project> init_projects(std::filesystem::path dir);
    public:
        Workspace(std::filesystem::path dir);
        virtual ~Workspace() = default;
        std::vector<Project>& get_projects();
};