#include "common.hpp"
#define SPDLOG_ACTIVE_LEVEL 2
#include <spdlog/spdlog.h>
#include "workspace.hpp"

std::vector<Project> Workspace::init_projects(std::filesystem::path dir) {
    std::vector<Project> result;
    for (auto const& file : std::filesystem::directory_iterator(dir)) {
        if (file.is_directory()) {
            result.emplace_back(file);
        }
    }
    std::sort(result.begin(), result.end(), [](Project& a, Project& b) {
        return a.get_number() < b.get_number();
    });
    return result;
}

Workspace::Workspace(std::filesystem::path dir):
    projects(init_projects(dir)) {
    SPDLOG_DEBUG("PRJKT workspace initialized [projects={}]", projects.size());
}

std::vector<Project>& Workspace::get_projects() {
    return projects;
}