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
            std::cout << "Источник данных: тестовый репозиторий\n\n";
        } else {
            repository = depviz::loadApkRepository(config.repository);
            std::cout << "Источник данных: Alpine Linux APKINDEX\n\n";
        }

        depviz::printDirectDependencies(config.package, repository);
        const auto graph = depviz::buildGraphDfs(
            config.package, repository, config.maxDepth
        );
        depviz::printGraph(graph, "Граф зависимостей (DFS без рекурсии)");

        const auto reverse = depviz::buildReverseGraph(
            config.package, repository, config.maxDepth
        );
        depviz::printReverseDependencies(reverse, config.package);

        depviz::writePlantUml(graph, "graph.puml");
        depviz::writeSvg(graph, config.package, "graph.svg");
        std::cout << "PlantUML сохранён: graph.puml\n";
        std::cout << "Изображение графа сохранено: graph.svg\n";

        if (config.asciiTree) {
            depviz::printAsciiTree(graph, config.package, config.maxDepth);
        }
        depviz::openSvg("graph.svg");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
