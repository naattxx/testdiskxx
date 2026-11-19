#include "src/part/ntfs_fix.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include "src/ui/intrfn.hpp"
#include <cassert>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

void repair_MFT_menu(const Component &root, disk_t &disk,
                     partition_t &partition, const int verbose,
                     const bool expert)
{
  auto expected_mft_state = mft_state::get(disk, partition, verbose, expert);

  if (!expected_mft_state)
  {
    display_message(root, expected_mft_state.error());
    return;
  }

  auto mft_state = *expected_mft_state;

  if (mft_state.good_mft && !mft_state.good_mft_mirror)
  {
    if (ask_confirmation("Fix MFT mirror using MFT ?"))
      display_message(root, mft_state.fix_mft_mirror());
    else
      log_info("Don't fix MFT mirror.");

    return;
  }
  if (mft_state.good_mft_mirror && !mft_state.good_mft)
  {
    if (ask_confirmation(
            "Fix MFT using its mirror ?\n"
            " - DANGEROUS NON REVERSIBLE OPERATION - \n"
            "Use it ONLY IF TestDisk and Windows failed to access this "
            "filesystem."
        ))
      display_message(root, mft_state.fix_mft());
    else
      log_info("Don't fix MFT.");

    return;
  }

  assert(expert &&
         "non expert user should recive an error message and abort repair");

  auto screen = App::Fullscreen();
  Element buttonDescription;
  auto buttonOptions = labled_button_options(buttonDescription);

  Components buttons {
      Button(
          "[  MFT  ]"
          "Fix MFT using MFT mirror",
          [&] -> void {
            display_message(root, mft_state.fix_mft());
            screen.Exit();
          },
          buttonOptions
      ),
      Button(
          "[MFT Mirror]"
          "Fix MFT mirror using MFT",
          [&] -> void {
            display_message(root, mft_state.fix_mft_mirror());
            screen.Exit();
          },
          buttonOptions
      ),
      Button("[  Quit  ]"
             "Return to NTFS functions",
             screen.ExitLoopClosure(), buttonOptions),
  };
  auto buttonsContainer =
      Container::Horizontal(buttons) | CatchEvent([&](const Event &e) -> bool {
        if (e == Event::q)
        {
          screen.Exit();
          return true;
        }
        return false;
      });

  auto dialog = Renderer(buttonsContainer, [&] -> Element {
    return vbox({
               mft_state.good_mft && mft_state.good_mft_mirror
                   ? text("Both MFT seem ok but they don't match.")
                   : text("MFT and MFT mirror are bad."),
               separator(),
               buttonsContainer->Render(),
           }) |
           size(WIDTH, ftxui::LESS_THAN, Terminal::Size().dimx * 0.75f) |
           borderHeavy | center;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));
}
