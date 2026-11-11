#include "depviz.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    try {
        if (argc > 2) {
            throw std::runtime_error("Использование: visualizer [путь_к_config.toml]");
        }
        const std::string configFile = argc == 2 ? argv[1] : "config.toml";
        const depviz::Config config = depviz::loadConfig(configFile);
        depviz::printConfig(config);

        depviz::Repository repository;
        if (config.testMode) {
            std::filesystem::path path(config.repository);
            if (path.is_relative()) {
                path = std::filesystem::path(configFile).parent_path() / path;
            }
            repository = depviz::loadTestRepository(path.string());
        } else {
            repository = depviz::loadApkRepository(config.repository);
        }

        const auto found = repository.find(config.package);
        if (found == repository.end()) {
            throw std::runtime_error("Пакет не найден: " + config.package);
        }
        std::cout << "Прямые зависимости пакета " << config.package << ":\n";
        for (const auto& dependency : found->second) {
            std::cout << "  " << dependency << '\n';
        }
        std::cout << '\n';

        const auto graph = depviz::buildGraphDfs(
            config.package, repository, config.maxDepth
        );
        depviz::printGraph(graph, "Граф зависимостей (DFS без рекурсии)");

        const auto reverse = depviz::buildReverseGraph(
            config.package, repository, config.maxDepth
        );
        depviz::printReverseDependencies(reverse, config.package);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
