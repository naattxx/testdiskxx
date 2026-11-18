#pragma once
#include "src/common.hpp"
#include <ftxui/component/app.hpp>

void repair_MFT_menu(const ftxui::Component &root, disk_t &disk,
                     partition_t &partition, const int verbose,
                     const bool expert);
