#include "config.h"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/table.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "src/fnctdsk.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include <chrono>
#include <cstddef>
#include <format>
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
    using std::string, std::to_string;
    string order = to_string(partition.order);
    string status =
        (partition.status == 'd' && partition.order == NO_ORDER)
            ? ""
            : to_string(partition.status);
    string partition_type =
        (partition.arch->get_partition_typename(partition) != nullptr)
            ? partition.arch->get_partition_typename(partition)
        : (partition.arch->get_part_type(partition) != 0)
            ? std::format("Sys={:02X}",
                          partition.arch->get_part_type(partition))
            : "Unknown";
    string start;
    string end;
    if (disk.unit == UNIT::SECTOR)
    {
      start = to_string(partition.part_offset / disk.sector_size);
      end   = to_string((partition.part_offset + partition.part_size - 1) /
                        disk.sector_size);
    }
    else
    {
      start = std::format("{:5} {:3} {:2}",
                          offset2cylinder(disk, partition.part_offset),
                          offset2head(disk, partition.part_offset),
                          offset2sector(disk, partition.part_offset));
      end   = std::format(
          "{:5} {:3} {:2}",
          offset2cylinder(disk,
                          partition.part_offset + partition.part_size - 1),
          offset2head(disk, partition.part_offset + partition.part_size - 1),
          offset2sector(disk, partition.part_offset + partition.part_size - 1)
      );
    }
    string size_in_sector = to_string(partition.part_size / disk.sector_size);
    string partname = (partition.partname.empty()) ? "" : "[" + partition.partname + "]";
    string fsname = (partition.fsname.empty()) ? "" : "[" + partition.fsname + "]";
    data.push_back({order, status, partition_type, start, end, size_in_sector, partname, fsname});
  }
  Table table(data);
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
