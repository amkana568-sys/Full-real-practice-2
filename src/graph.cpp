#include "depviz.hpp"

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace depviz {

Graph buildGraphDfs(
    const std::string& start,
    const Repository& repository,
    int maxDepth
) {
    if (repository.find(start) == repository.end()) {
        throw std::runtime_error("Пакет не найден в репозитории: " + start);
    }
    if (maxDepth < 0) {
        throw std::runtime_error("Глубина анализа не может быть отрицательной.");
    }

    Graph graph;
    std::map<std::string, int> bestDepth;
    std::vector<std::pair<std::string, int>> stack = {{start, 0}};
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        const std::string& node = current.first;
        const int depth = current.second;
        if (depth > maxDepth) {
            continue;
        }
        const auto visited = bestDepth.find(node);
        if (visited != bestDepth.end() && visited->second <= depth) {
            continue;
        }
        bestDepth[node] = depth;

        std::vector<std::string> dependencies;
        if (depth < maxDepth) {
            const auto found = repository.find(node);
            if (found != repository.end()) {
                dependencies = found->second;
            }
        }
        graph[node] = dependencies;
        for (auto it = dependencies.rbegin(); it != dependencies.rend(); ++it) {
            stack.push_back({*it, depth + 1});
        }
    }
    return graph;
}

Graph buildReverseGraph(
    const std::string& start,
    const Repository& repository,
    int maxDepth
) {
    Repository reverse;
    for (const auto& item : repository) {
        reverse[item.first];
        for (const auto& dependency : item.second) {
            reverse[dependency].push_back(item.first);
        }
    }
    return buildGraphDfs(start, reverse, maxDepth);
}

void printReverseDependencies(const Graph& graph, const std::string& root) {
    std::cout << "Обратные зависимости пакета " << root << ":\n";
    bool foundAny = false;
    for (const auto& item : graph) {
        if (item.first == root) {
            continue;
        }
        std::cout << "  " << item.first << '\n';
        foundAny = true;
    }
    if (!foundAny) {
        std::cout << "  нет\n";
    }
    std::cout << '\n';
}

void printGraph(const Graph& graph, const std::string& title) {
    std::cout << title << ":\n";
    for (const auto& item : graph) {
        std::cout << "  " << item.first << " -> [";
        for (std::size_t i = 0; i < item.second.size(); ++i) {
            if (i != 0) {
                std::cout << ", ";
            }
            std::cout << item.second[i];
        }
        std::cout << "]\n";
    }
    std::cout << '\n';
}

}  // namespace depviz
