#ifndef _INTRFN_H
#define _INTRFN_H

#include "ftxui/component/app.hpp"
#include <string_view>

auto ask_confirmation(std::string_view msg) -> bool;

/*@ requires valid_read_string(msg); */
void display_message(const ftxui::Component &root, std::string_view msg);

#endif
