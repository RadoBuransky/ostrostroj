#include "common.hpp"
#include "workspace.hpp"

Workspace::Workspace(const std::filesystem::path _dir):
    dir(_dir) {
}

std::vector<Project2>& Workspace::get_projects() {
    return projects;
}