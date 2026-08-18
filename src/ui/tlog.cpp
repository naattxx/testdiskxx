#include <cpptui.hpp>
#include <memory>

#include "config.h"
#include "src/log.hpp"
#include "tlog.hpp"

using namespace cpptui;

auto ask_testdisk_log_creation(App &app) -> TD_LOG
{
  TD_LOG log_choice = TD_LOG::NONE;

  auto dialog                 = std::make_shared<Dialog>(&app);
  auto dialog_v               = std::make_shared<Vertical>();
  dialog_v->responsive_height = false;
  dialog->set_title("Log creation");
  dialog->modal = true;
  dialog->open();

  auto aboutBuild =
      std::make_shared<Label>(StyledText("TestDisk++ ")
                                  .bold(VERSION)
                                  .add(", Data Recovery Utility, ")
                                  .add(TESTDISKDATE));
  auto author  = std::make_shared<Label>("naattxx <grenier@cgsecurity.org>");
  auto website = std::make_shared<Label>("https://www.github.com/");

  auto spacer  = std::make_shared<VerticalSpacer>();
  auto spacer1 = std::make_shared<VerticalSpacer>(1);
  auto spacer3 = std::make_shared<VerticalSpacer>(3);

  auto aboutTestdisk = std::make_shared<Paragraph>(
      "TestDisk is free data recovery software designed to help recover lost "
      "partitions and/or make non-booting disks bootable again when these "
      "symptoms are caused by faulty software, certain types of viruses or "
      "human error.\nIt can also be used to repair some filesystem errors.\n"
  );
  aboutTestdisk->max_width = 75;

  auto aboutLog = std::make_shared<Paragraph>(
      StyledText("Information gathered during TestDisk use can be recorded for "
                 "later review. If you choose to create the text file, ")
          .bold("testdisk.log")
          .add(", it will contain TestDisk options, technical information and "
               "various outputs; including any folder/file names TestDisk was "
               "used to find and list onscreen.\n")
  );
  aboutLog->max_width = 75;

  auto createBtn =
      std::make_shared<Button>(StyledText(" [ Create ] ")
                                   .colored("Create a new log file",
                                            Theme::current().secondary),
                               [&app, &log_choice]() -> void {
                                 log_choice = TD_LOG::CREATE;
                                 app.quit();
                               });
  createBtn->fixed_width = utf8_display_width(createBtn->get_label());

  auto appendBtn =
      std::make_shared<Button>(StyledText(" [ Append ] ")
                                   .colored("Append information to log file",
                                            Theme::current().secondary),
                               [&app, &log_choice]() -> void {
                                 log_choice = TD_LOG::APPEND;
                                 app.quit();
                               });
  appendBtn->fixed_width = utf8_display_width(appendBtn->get_label());

  auto noLogBtn =
      std::make_shared<Button>(StyledText(" [ No Log ] ")
                                   .colored("Don't record anything",
                                            Theme::current().secondary),
                               [&app, &log_choice]() -> void {
                                 log_choice = TD_LOG::NONE;
                                 app.quit();
                               });
  noLogBtn->fixed_width = utf8_display_width(noLogBtn->get_label());

  dialog_v->add(aboutBuild);
  dialog_v->add(author);
  dialog_v->add(website);
  dialog_v->add(spacer3);
  dialog_v->add(aboutTestdisk);
  dialog_v->add(spacer1);
  dialog_v->add(aboutLog);
  dialog_v->add(spacer1);
  dialog_v->add(createBtn);
  dialog_v->add(appendBtn);
  dialog_v->add(noLogBtn);
  dialog_v->add(spacer);

  dialog->add(dialog_v);

  auto root = std::make_shared<Stack>();
  root->add(dialog);

  app.run(root);

  dialog->close();

  return log_choice;
}
