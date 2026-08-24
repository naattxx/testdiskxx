#include "config.h"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/table.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include <chrono>
#include <cstddef>
#include <future>
#include <string>
#include <unistd.h>
#include <vector>

using namespace ftxui;

auto getPartitionsTable(const disk_t &disk, const list_part_t &partitions)
    -> Element
{
  std::vector<std::vector<std::string>> data{
      {"", "", "Partition", "Start", "End", "Size in sectors", "", ""}
  };

  for (const auto &partition : partitions)
  {
    data.push_back(aff_part_aux(AFF_PART_ORDER | AFF_PART_STATUS, disk, partition));
  }
  Table table(data);
  table.SelectColumns(3, 5).Decorate(align_right);
  table.SelectRow(0).Decorate(bold);
  table.SelectRow(0).Separator(EMPTY);
  return table.Render();
}

auto interface_analyse(disk_t &disk, const int verbose, const int save_header)
    -> list_part_t
{
  std::shared_future<list_part_t> list_part;
  auto screen = ftxui::App::Fullscreen();
  Component root;
  auto buttonDescription = text("");
  auto buttonOptions     = ButtonOption();
  buttonOptions.transform =
      [&buttonDescription](const EntryState &s) -> Element {
    if (s.focused)
    {
      buttonDescription = text(s.label.substr(s.label.find(']') + 1));
      return hflow({
          text(">" + s.label.substr(0, s.label.find(']') + 1)) |
              bgcolor(Color::White) | color(Color::Black) | bold,
          text(" "),
      });
    }

    return text(" " + s.label.substr(0, s.label.find(']') + 1) + " ");
  };
  auto options = Container::Horizontal({
      Button("[ Quit        ]"
             "Return to menu",
             screen.ExitLoopClosure(), buttonOptions),
      Button(
          "[ Quick Search ]"
          "Analyse current partition structure and search for lost "
          "partitions",
          []() {}, buttonOptions
      ),
      Button(
          "[ Backup       ]"
          "Filesystem Utils",
          []() {}, buttonOptions
      ),
  });
  size_t frame = 0;

  root = Renderer(options, [&]() {
    bool loaded = list_part.valid() &&
                  list_part.wait_for(std::chrono::milliseconds(10)) ==
                      std::future_status::ready;
    if (!loaded)
    {
      screen.RequestAnimationFrame();
      frame++;
    }

    return vbox({
        hflow({text("TestDisk++ "), bold(text(VERSION)),
               text(", Data Recovery Utility, "), text(TESTDISKDATE)}),
        text("naattxx"),
        text("https://github.com/naattxx/testdiskxx"),
        separatorEmpty(),
        text(disk.description(disk)),
        (loaded) ? vbox({
                       text("Current partition structure:"),
                       getPartitionsTable(disk, list_part.get()),
                       filler(),
                       (disk.arch->msg_part_type != nullptr)
                           ? text(disk.arch->msg_part_type)
                           : emptyElement(),
                       options->Render(),
                       buttonDescription | hcenter,
                   })
                 : hflow({
                       text("Checking current partition structure "),
                       spinner(15, frame),
                   }),
    });
  });

  list_part = std::async([&]() -> list_part_t {
    log_info("\nAnalyse {}", disk.description(disk));
    list_part_t list = disk.arch->read_part(disk, verbose, save_header);
    log_info("Current partition structure:");
    screen_buffer_to_log();
    return list;
  });

  screen.Loop(root);

  return list_part.get();
}
