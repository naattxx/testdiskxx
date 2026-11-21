#ifndef _INTRFN_H
#define _INTRFN_H

#include "ftxui/component/app.hpp"
#include "src/common.hpp"
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <string_view>

[[nodiscard]]
auto partition_header(const disk_t &disk, const partition_t &partition) noexcept
    -> ftxui::Element;

[[nodiscard]]
auto labled_button_options(ftxui::Element &buttonDescription) -> ftxui::ButtonOption;

[[nodiscard]]
auto ask_confirmation(std::string_view msg) -> bool;
[[nodiscard]]
auto ask_log_location(const std::string &failed_filename) -> std::string;

/*@ requires valid_read_string(msg); */
void display_message(const ftxui::Component &root, std::string_view msg);

#endif
