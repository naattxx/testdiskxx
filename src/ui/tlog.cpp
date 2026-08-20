#include "tlog.hpp"
#include "config.h"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "src/log.hpp"
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace ftxui;

auto ask_testdisk_log_creation(App &app) -> TD_LOG
{
  std::vector<std::string> entries = {
      "[ Create ] Create a new log file",
      "[ Append ] Append information to log file",
      "[ No Log ] Don't record anything",
  };
  int selected = 0;
  MenuOption option;
  option.on_enter = app.ExitLoopClosure();
  auto menu       = Menu(&entries, &selected, option);

  Elements aboutLog;
  { // workaround for in paragraph bold text
    using namespace std::literals;
    namespace views = std::views;
    aboutLog.append_range(
        "Information gathered during TestDisk use can be recorded for "
        "later review. If you choose to create the text file, testdisk.log , it will contain "
        "TestDisk options, technical "
        "information and "
        "various outputs; including any folder/file names "
        "TestDisk was "
        "used to find and list onscreen."sv |
        views::split(" "sv) | views::transform([](auto &&stri) {
          auto str = std::string(std::string_view(stri));
          if (str == "testdisk.log")
            return bold(text(str));
          return text(str + " ");
        })
    );
  }
  auto dialog = Renderer(menu, [&]() {
    return window(text("Log creation"),
                  vbox({
                      hflow({text("TestDisk++ "), bold(text(VERSION)),
                             text(", Data Recovery Utility, "),
                             text(TESTDISKDATE)}),
                      text("naattxx <grenier@cgsecurity.org>"),
                      text("https://www.github.com/"),
                      separator(),
                      paragraph(
                          "TestDisk is free data recovery software designed "
                          "to "
                          "help recover lost "
                          "partitions and/or make non-booting disks bootable "
                          "again when these "
                          "symptoms are caused by faulty software, certain "
                          "types of viruses or "
                          "human error.\nIt can also be used to repair some "
                          "filesystem errors."
                      ),
                      separatorEmpty(),
                      hflow(aboutLog),
                      separatorEmpty(),
                      text("Use arrow keys to select, then press Enter key:"),
                      menu->Render(),
                  })) |
           size(WIDTH, ftxui::LESS_THAN, 90) | center;
  });

  app.Loop(dialog);

  if (selected == 0)
    return TD_LOG::CREATE;
  if (selected == 1)
    return TD_LOG::APPEND;
  return TD_LOG::NONE;
}
