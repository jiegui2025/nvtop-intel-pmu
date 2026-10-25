cmake_minimum_required(VERSION 3.24)
project(intel_pmu_package_consumer LANGUAGES C)

find_package(intel_pmu CONFIG REQUIRED)

add_executable(intel_pmu_package_consumer main.c)
target_link_libraries(intel_pmu_package_consumer PRIVATE intel_pmu::intel_pmu)
