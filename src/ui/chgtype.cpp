#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/mouse.hpp"
#include "ftxui/dom/elements.hpp"
#include "src/common.hpp"
#include "src/guid_cmp.hpp"
#include "src/guid_cpy.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"
#include "src/part/partgpt.hpp"
#include <array>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace ftxui;

extern const arch_fnct_t arch_none;
extern const arch_fnct_t arch_gpt;
extern const arch_fnct_t arch_i386;
extern const arch_fnct_t arch_sun;
extern const std::array<const struct systypes_gtp, 46> gpt_sys_types;

static void change_part_type_int(const Component root, const disk_t &disk_car,
                                 partition_t &partition)
{
  if (partition.arch->set_part_type == nullptr)
    return;
  auto screen = App::Fullscreen();

  std::array<Elements, 3> columns;

  /* Create an index of all partition type except Intel extended */
  std::vector<std::string> avalable_parts;
  int current_part{0};
  {
    partition_t new_partition = partition;
    for (int i = 0; i <= 0xFF; i++)
    {
      if (partition.arch->set_part_type(new_partition, i) == 0)
      {
        std::string_view name =
            new_partition.arch->get_partition_typename(new_partition);
        if (!name.empty())
        {
          avalable_parts.push_back(std::format(" {:02x} {} ",i, name));

          if (partition.arch->get_part_type(partition) == i)
            current_part = avalable_parts.size() - 1;
        }
      }
    }

    int thirdRoundedUp = ((avalable_parts.size() + 3) - 1) / 3;
    for (const auto &[i, name] : avalable_parts | std::views::enumerate)
    {
      columns[i / thirdRoundedUp].push_back(text(name));
    }
  }

  std::string out;
  Component input =
      Input(&out, std::format("[current is {:02x}]", current_part),
            {.multiline = false}) |
      CatchEvent([&](Event event) -> bool {
        if (event == Event::Return && !out.empty())
        {
          int numOut = std::stoi(out, nullptr, 16);
          partition.arch->set_part_type(partition, numOut);
          screen.Exit();
          return true;
        }

        return (event.is_character() && (!isxdigit(event.character()[0]) || out.size() >= 2));
      });

  auto dialog = Renderer(input, [&]() -> Element {
    return vbox({
               text("List of partition types:"),
               separatorDashed(),
               hbox({
                   vbox(columns[0]),
                   vbox(columns[1]),
                   vbox(columns[2]),
               }),
               separatorEmpty(),
               hflow({
                   text("   New partition type ? "),
                   input->Render() | size(WIDTH, EQUAL, 20),
               }),
           }) |
           border;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  partition.arch->set_part_type(partition, std::stoi(out, nullptr, 16));
}

static void change_part_type_list(const Component root, const disk_t &disk_car,
                                  partition_t &partition)
{
  if (partition.arch->set_part_type == nullptr)
    return;
  auto screen = App::Fullscreen();

  std::array<Components, 3> columns;
  int row{0}, col{0};

  int thirdRoundedUp;

  /* Create an index of all partition type except Intel extended */
  std::vector<std::pair<int, std::string_view>> avalable_parts;
  {
    partition_t new_partition = partition;
    int current_part{0};
    for (int i = 0; i <= 0xFF; i++)
    {
      if (partition.arch->set_part_type(new_partition, i) == 0)
      {
        std::string_view name =
            new_partition.arch->get_partition_typename(new_partition);
        if (!name.empty())
        {
          avalable_parts.emplace_back(i, name);

          if (partition.arch->get_part_type(partition) == i)
            current_part = avalable_parts.size() - 1;
        }
      }
    }
    thirdRoundedUp = ((avalable_parts.size() + 3) - 1) / 3;
    auto dv        = std::div(current_part, thirdRoundedUp);
    row            = dv.rem;
    col            = dv.quot;
    for (const auto &[i, name] : avalable_parts | std::views::enumerate)
    {
      columns[i / thirdRoundedUp].push_back(MenuEntry(name.second));
    }
  }

  auto grid = Container::Horizontal(
      {
          Container::Vertical(columns[0], &row) | size(WIDTH, GREATER_THAN, 17),
          Container::Vertical(columns[1], &row) | size(WIDTH, GREATER_THAN, 17),
          Container::Vertical(columns[2], &row) | size(WIDTH, GREATER_THAN, 17),
      },
      &col
  );

  grid |= CatchEvent([&](Event e) -> bool {
    if (e == Event::Return ||
        (e.is_mouse() && e.mouse().button == Mouse::Button::Left &&
         e.mouse().motion == Mouse::Pressed))
    {
      screen.Exit();
      return true;
    }
    return false;
  });

  auto dialog = Renderer(grid, [&]() -> Element {
    return vbox({
               text("Please choose the partition type, press Enter when done:"),
               separatorDashed(),
               vbox(grid->Render()) | vscroll_indicator | yframe,
           }) |
           border;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  partition.arch->set_part_type(
      partition, avalable_parts[row + col * thirdRoundedUp].first
  );
}

static void gpt_change_part_type(const Component root, const disk_t &disk_car,
                                 partition_t &partition)
{
  log_info("gpt_change_part_type\n");

  auto screen = App::Fullscreen();

  int row{0}, col{0};
  auto grid = Container::Horizontal(
      {
          Container::Vertical({}, &row),
          Container::Vertical({}, &row),
          Container::Vertical({}, &row),
      },
      &col
  );

  int thirdRoundedUp = ((gpt_sys_types.size() + 3) - 1) / 3;
  for (const auto &[i, gpt_sys_type] : gpt_sys_types | std::views::enumerate)
  {

    grid->ChildAt(i / thirdRoundedUp)->Add(MenuEntry(gpt_sys_type.name));

    /* By default, select the current type */
    if (partition.part_type_gpt == gpt_sys_type.part_type)
    {
      auto dv = std::div(static_cast<int>(i), thirdRoundedUp);
      row     = dv.rem;
      col     = dv.quot;
    }
  }

  grid |= CatchEvent([&](Event e) -> bool {
    if (e == Event::Return ||
        (e.is_mouse() && e.mouse().button == Mouse::Button::Left &&
         e.mouse().motion == Mouse::Pressed))
    {
      screen.Exit();
      return true;
    }
    return false;
  });

  auto dialog = Renderer(grid, [&]() -> Element {
    return vbox({
               text("Please choose the partition type, press Enter when done:"),
               separatorDashed(),
               vbox(grid->Render()) | vscroll_indicator | yframe,
           }) |
           border;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  guid_cpy(&partition.part_type_gpt,
           &gpt_sys_types[row + col * thirdRoundedUp].part_type);
}

void change_part_type_interface(const Component root, const disk_t &disk_car,
                                partition_t &partition)
{
  if (partition.arch == nullptr)
  {
    log_error("change_part_type_interface arch==nullptr");
    return;
  }
  if (partition.arch == &arch_gpt)
  {
    gpt_change_part_type(root, disk_car, partition);
    log_info("Change partition type:");
    log_partition(disk_car, partition);
    partition.arch = &arch_none;
    change_part_type_list(root, disk_car, partition);
    log_info("Change partition type:");
    log_partition(disk_car, partition);
    partition.arch = &arch_gpt;
    return;
  }
  if (partition.arch == &arch_i386)
  {
    change_part_type_int(root, disk_car, partition);
    log_info("Change partition type:");
    log_partition(disk_car, partition);
    partition.arch = &arch_none;
    change_part_type_list(root, disk_car, partition);
    log_info("Change partition type:");
    log_partition(disk_car, partition);
    partition.arch = &arch_i386;
    return;
  }
  if (partition.arch->set_part_type == nullptr)
  {
    log_error("change_part_type_interface set_part_type==nullptr");
    return;
  }
  if (partition.arch == &arch_sun)
    change_part_type_int(root, disk_car, partition);
  else
    change_part_type_list(root, disk_car, partition);
  log_info("Change partition type:");
  log_partition(disk_car, partition);
}
