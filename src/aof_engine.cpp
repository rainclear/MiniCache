#include "minicache/aof_engine.hpp"
#include "minicache/command_factory.hpp"
#include <fstream>
#include <iostream>

namespace minicache {

AofEngine::AofEngine(std::string filename) : filename_(std::move(filename)) {
    // Open in append mode
    file_.open(filename_, std::ios::out | std::ios::app);
}

AofEngine::~AofEngine() {
    sync();
    if (file_.is_open()) {
        file_.close();
    }
}

void AofEngine::append(const std::string& raw_cmd) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_ << raw_cmd << "\n";
        file_.flush(); // Flush to disk for crash resilience
    }
}

std::size_t AofEngine::load(CacheStore& store) {
    std::ifstream input_file(filename_);
    if (!input_file.is_open()) {
        return 0; // File does not exist yet
    }

    std::size_t replayed_count = 0;
    std::string line;
    while (std::getline(input_file, line)) {
        if (line.empty()) continue;

        auto cmd = CommandFactory::parse(line);
        if (cmd) {
            cmd->execute(store);
            ++replayed_count;
        }
    }
    return replayed_count;
}

void AofEngine::sync() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace minicache