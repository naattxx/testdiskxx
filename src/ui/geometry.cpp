#include "geometry.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include <array>
#include <cctype>
#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

using namespace ftxui;

constexpr int MAX_HEADS{255};

auto change_geometry(const Component root, disk_t &disk) -> int
{
  auto screen = App::Fullscreen();
  std::string cylinders;
  std::string heads;
  std::string sectors;

  std::array sectorSizes{1u, 256u, 512u, 1024u, 3 * 512u, 2048u, 4096u, 8192u};
  int selectedSectorSize{0};

  for (auto [index, sector_size] : std::views::enumerate(sectorSizes))
  {
    if (sector_size == disk.sector_size)
      selectedSectorSize = index;
  }

  auto cylindrInput =
      Input(&cylinders, std::to_string(disk.geom.cylinders),
            {.multiline = false}) |
      CatchEvent([&](Event event) -> bool {
        return event.is_character() && !std::isdigit(event.character()[0]);
      });

  auto headsInput =
      Input(&heads, std::to_string(disk.geom.heads_per_cylinder),
            {.multiline = false,
             .on_change = [&]() -> void {
               if (heads.empty())
                 return;

               int num = std::stoi(heads);
               if (num > MAX_HEADS)
                 heads = std::to_string(MAX_HEADS);
               else if (num < 1)
                 heads = '1';
               else
                 heads = std::to_string(std::stoi(heads));
             }}) |
      CatchEvent([&](Event event) -> bool {
        return event.is_character() && !std::isdigit(event.character()[0]);
      });

  auto sectorsInput =
      Input(&sectors, std::to_string(disk.geom.sectors_per_head),
            {.multiline = false,
             .on_change = [&]() -> void {
               if (sectors.empty())
                 return;

               int num = std::stoi(sectors);
               if (num > 63)
                 sectors = "63";
               else if (num < 1)
                 sectors = '1';
               else
                 sectors = std::to_string(std::stoi(sectors));
             }}) |
      CatchEvent([&](Event event) -> bool {
        return event.is_character() && !std::isdigit(event.character()[0]);
      });

  auto sectorSizeToggle =
      Toggle(std::vector<std::string>{"1", "256", "512", "1024", "(3*512)",
                                      "2048", "4096", "8192"},
             &selectedSectorSize);

  auto okButton = Button("Ok ", screen.ExitLoopClosure(),
                         {.transform = [](const EntryState &s) -> Element {
                           if (s.focused)
                           {
                             return text(">" + s.label) |
                                    bgcolor(Color::White) |
                                    color(Color::Black) | bold;
                           }

                           return text(" " + s.label);
                         }});
  bool cancel{false};
  auto cancelButton = Button("Cancel ",
                             [&]() -> void {
                               cancel = true;
                               screen.Exit();
                             },
                             {.transform = [](const EntryState &s) -> Element {
                               if (s.focused)
                               {
                                 return text(">" + s.label) |
                                        bgcolor(Color::White) |
                                        color(Color::Black) | bold;
                               }

                               return text(" " + s.label);
                             }});

  auto container = Container::Vertical({
      cylindrInput,
      headsInput,
      sectorsInput,
      sectorSizeToggle,
      Container::Horizontal({
          okButton,
          cancelButton,
      }),
  });
  auto dialog    = Renderer(container, [&]() -> Element {
    return vbox({
               text(disk.description(disk)),
               separator(),
               paragraph("Because these numbers change the way that TestDisk "
                         "looks for partitions "
                         "and calculates their sizes, it's important to have "
                         "the correct disk geometry.\n"
                         "PC partitioning programs often make partitions end "
                         "on cylinder boundaries."),
               separatorEmpty(),
               paragraph(
                   "A partition's CHS values are based on disk translations "
                   "which make them "
                   "different than its physical geometry. The most common CHS "
                   "head values are: 255, 240 and sometimes 16."
               ),
               separatorEmpty(),
               hflow({text(" Cylinders                              : "),
                      cylindrInput->Render()}),
               hflow({text(" Heads                                  : "),
                      headsInput->Render()}),
               hflow({text(" Sectors                                : "),
                      sectorsInput->Render()}),
               hflow({text(" Sector Size "),
                      text("(WARNING: VERY DANGEROUS!)") | color(Color::Yellow),
                      text(" : "), sectorSizeToggle->Render()}),
               separator(),
               hflow({okButton->Render(), cancelButton->Render()}) | hcenter,
           }) |
           border | center;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  if (cancel)
    return 0;

  bool geo_modified{false};

  if (!cylinders.empty())
  {
    disk.geom.cylinders = std::stoi(cylinders);
    geo_modified        = true;
  }

  if (!heads.empty())
  {
    disk.geom.heads_per_cylinder = std::stoi(heads);
    geo_modified                 = true;
  }

  if (!sectors.empty())
  {
    disk.geom.sectors_per_head = std::stoi(sectors);
    geo_modified               = true;
  }
  if (sectorSizes[selectedSectorSize] != disk.sector_size)
  {
    if (disk.set_sector_size(sectorSizes[selectedSectorSize]))
    {
      log_critical("Illegal sector size");
    }
    geo_modified = true;
  }

  if (cylinders.empty())
    disk.set_cylinders_from_size_up();

  if (geo_modified)
  {
    disk.disk_size = static_cast<uint64_t>(disk.geom.cylinders) *
                     disk.geom.heads_per_cylinder * disk.geom.sectors_per_head *
                     disk.sector_size;
#ifdef __APPLE__
    // On MacOSX if HD contains some bad sectors, the disk size may not be
    // correctly detected
    disk.disk_real_size = disk.disk_size;
#endif
    log_info("New geometry:");
    log_info("{} sector_size={}", disk.description(disk), disk.sector_size);
    disk.autoset_unit();

    if (sectorSizes[selectedSectorSize] != disk.sector_size)
      return 1;
  }

  return 0;
}
