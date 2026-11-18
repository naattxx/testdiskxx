#ifndef _INTRFN_H
#define _INTRFN_H

#include "ftxui/component/app.hpp"
#include <string>
#include <string_view>

[[nodiscard]]
auto ask_confirmation(std::string_view msg) -> bool;
[[nodiscard]]
auto ask_log_location(const std::string &failed_filename) -> std::string;

/*@ requires valid_read_string(msg); */
void display_message(const ftxui::Component &root, std::string_view msg);

#endif
