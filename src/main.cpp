#include "depviz.hpp"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        if (argc > 2) {
            throw std::runtime_error("Использование: visualizer [путь_к_config.toml]");
        }
        const std::string filename = argc == 2 ? argv[1] : "config.toml";
        const depviz::Config config = depviz::loadConfig(filename);
        depviz::printConfig(config);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}
