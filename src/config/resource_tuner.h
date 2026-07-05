#pragma once
#include <thread>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>

namespace config {

enum class HardwareTier { Constrained, Performance, Beast };

struct TuningProfile {
    HardwareTier tier;
    int threadNum;
    size_t dbConnections;
    bool enableInMemCache;

    static TuningProfile detect(int manualThreads, size_t manualDbConnections) {
        double cpu_quota = -1.0;
        std::ifstream quota_file("/sys/fs/cgroup/cpu.max"); 
        if (quota_file.is_open()) {
            std::string quota_str, period_str;
            if (quota_file >> quota_str >> period_str) {
                try {
                    if (quota_str != "max") {
                        cpu_quota = std::stod(quota_str) / std::stod(period_str);
                    }
                } catch (...) {
                }
            }
        }

        unsigned int cores = std::thread::hardware_concurrency();
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        long total_ram_gb = (pages * page_size) / (1024 * 1024 * 1024);

        TuningProfile p;
        
        // If we are restricted by Docker (cpu_quota <= 1.0) or have very little RAM, 
        // we MUST use Constrained mode to survive the 0.4GB limit.
        if ((cpu_quota > 0.0 && cpu_quota <= 1.1) || total_ram_gb < 2) {
            p.tier = HardwareTier::Constrained;
            p.threadNum = manualThreads; 
            p.dbConnections = manualDbConnections; // Let compose control this
            p.enableInMemCache = false;
        } else if (cores <= 8) {
            p.tier = HardwareTier::Performance;
            p.threadNum = (int)cores;
            p.dbConnections = 40; // Conservative to avoid exhaustion
            p.enableInMemCache = true;
        } else {
            p.tier = HardwareTier::Beast;
            p.threadNum = (int)cores;
            p.dbConnections = 45; // Max safe value for 2 instances vs 100 limit
            p.enableInMemCache = true;
        }
        return p;
    }
};

} // namespace config
