
// Copyright (c) 2023 dingodb.com, Inc. All Rights Reserved
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <termios.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "CLI/CLI.hpp"
#include "brpc/channel.h"
#include "brpc/controller.h"
#include "bthread/bthread.h"
#include "client_v2/cli_state.h"
#include "client_v2/coordinator.h"
#include "client_v2/document_index.h"
#include "client_v2/dump.h"
#include "client_v2/helper.h"
#include "client_v2/interation.h"
#include "client_v2/kv.h"
#include "client_v2/meta.h"
#include "client_v2/restore.h"
#include "client_v2/store.h"
#include "client_v2/tools.h"
#include "client_v2/vector_index.h"
#include "common/cli_options.h"
#include "common/helper.h"
#include "common/logging.h"
#include "common/version.h"
#include "fmt/core.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "proto/common.pb.h"

const std::string kProgramName = "dingodb_cli";
const std::string kProgramDesc = "dingodb-cli client tool.";

struct RootCliOptions {
  std::string format = "auto";
};

void SetUpRootOptions(CLI::App& app, RootCliOptions& root_options) {
  auto& options = dingodb::cli::GetOptions();

  auto* format_option =
      app.add_option("--format", root_options.format, "Output format: auto|table|plain|json")
          ->default_val("auto")
          ->check(CLI::IsMember({"auto", "table", "plain", "json"}, CLI::ignore_case));
  format_option->each(
      [](const std::string& value) { dingodb::cli::GetOptions().format = dingodb::cli::ParseOutputFormat(value); });

  app.add_flag("--no-color", options.no_color, "Disable colored table output.");
  app.add_flag("--quiet", options.quiet, "Reduce non-essential output.");
  app.add_option("--timeout-ms,--timeout", options.timeout_ms, "RPC timeout in milliseconds.")->default_val(60000);
  app.add_option("--retry", options.max_retry, "RPC retry count.")->default_val(5);
  app.add_flag("--json-pretty", options.json_pretty, "Pretty print JSON output.");
}

void BootstrapFormatFromArgv(int argc, char* argv[]) {
  auto& options = dingodb::cli::GetOptions();
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument.rfind("--format=", 0) == 0) {
      options.format = dingodb::cli::ParseOutputFormat(argument.substr(std::string("--format=").size()));
      return;
    }
    if (argument == "--format" && i + 1 < argc) {
      options.format = dingodb::cli::ParseOutputFormat(argv[i + 1]);
      return;
    }
  }
}

void PrintSubcommandHelp(const CLI::App& app, const std::string& subcommand_name) {
  try {
    CLI::App* subcommand = app.get_subcommand(subcommand_name);
    std::cout << subcommand->help() << std::endl;
  } catch (const CLI::OptionNotFound& e) {
    std::cerr << "\n >Not found command: " << subcommand_name << std::endl;
  }
}

char Getch() {
  struct termios oldt, newt;
  char ch;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  // Disable the echo and key buffering
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  ch = getchar();

  // Restore the previous terminal properties
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

void DisplayInput(const std::string& current_input, int cursor_position) {
  std::cout << "\r" << std::string(80, ' ') << "\r >" << current_input;
  for (int i = 0; i < current_input.length() - cursor_position; i++) {
    std::cout << "\b";  // Move cursor left
  }
  std::cout.flush();
}

void ProcessInput(std::string& current_input, std::vector<std::string>& history, int& history_index,
                  int& cursor_position) {
  char ch;
  while (true) {
    DisplayInput(current_input, cursor_position);

    ch = Getch();
    // ESC
    if (ch == 27) {
      // Start Identifier Arrow key input
      char next1 = Getch();
      if (next1 == 91) {  // [
        char next2 = Getch();
        if (next2 == 'A') {  // Up Arrow
          if (history_index + 1 < history.size()) {
            history_index++;
            current_input = history[history.size() - 1 - history_index];
            cursor_position = current_input.length();
          }
        } else if (next2 == 'B') {  // Down Arrow
          if (history_index > 0) {
            history_index--;
            current_input = history[history.size() - 1 - history_index];
            cursor_position = current_input.length();
          } else if (history_index == 0) {
            history_index = -1;  // Reset history index
            current_input.clear();
            cursor_position = 0;
          }
        } else if (next2 == 'C') {  // Right Arrow
          if (cursor_position < current_input.length()) {
            cursor_position++;
          }
        } else if (next2 == 'D') {  // Left Arrow
          if (cursor_position > 0) {
            cursor_position--;
          }
        }
        continue;
      }
    } else if (ch == '\n') {  // Enter
      if (!current_input.empty()) {
        history.push_back(current_input);
        // std::cout << "\nSubmitted: " << current_input << std::endl;
      }
      return;
    } else if (ch == 127) {  // Backspace
      if (cursor_position > 0) {
        current_input.erase(cursor_position - 1, 1);  // delete char
        cursor_position--;
      }
    } else {
      current_input.insert(current_input.begin() + cursor_position, ch);
      cursor_position++;
    }
  }
}

int InteractiveCli(CLI::App& app) {
  std::vector<std::string> history_commands;
  std::string input;
  int history_index = -1;
  int cursor_position = 0;

  while (true) {
    input.clear();
    cursor_position = 0;
    ProcessInput(input, history_commands, history_index, cursor_position);

    if (input.empty()) {
      continue;
    }

    if (input == "exit" || input == "quit") {
      break;
    }

    if (input == "help") {
      std::cout << app.help() << std::endl;
      continue;
    } else if (input.rfind("help", 0) == 0) {
      std::string subcommand_name = input.substr(5);
      if (subcommand_name.empty()) {
        std::cout << app.help() << std::endl;
      } else {
        PrintSubcommandHelp(app, subcommand_name);
      }
      continue;
    }

    std::vector<std::string> args = {"dingodb_cli"};
    std::istringstream iss(input);
    for (std::string s; iss >> s;) {
      args.push_back(s);
    }

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& arg : args) {
      argv.push_back(arg.data());
    }
    argv.push_back(nullptr);
    try {
      app.parse(argv.size() - 1, argv.data());
    } catch (const CLI::ParseError& e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

  return 0;
}

void InitLog(const std::string& log_dir) {
  std::error_code error_code;
  std::filesystem::create_directories(log_dir, error_code);

  FLAGS_logbufsecs = 0;
  FLAGS_stop_logging_if_full_disk = true;
  FLAGS_minloglevel = google::GLOG_INFO;
  FLAGS_logbuflevel = google::GLOG_INFO;
  FLAGS_logtostdout = false;
  FLAGS_logtostderr = false;
  FLAGS_alsologtostderr = false;

  google::InitGoogleLogging(kProgramName.c_str());
  google::SetLogDestination(google::GLOG_INFO, fmt::format("{}/{}.info.log.", log_dir, kProgramName).c_str());
  google::SetLogDestination(google::GLOG_WARNING, fmt::format("{}/{}.warn.log.", log_dir, kProgramName).c_str());
  google::SetLogDestination(google::GLOG_ERROR, fmt::format("{}/{}.error.log.", log_dir, kProgramName).c_str());
  google::SetLogDestination(google::GLOG_FATAL, fmt::format("{}/{}.fatal.log.", log_dir, kProgramName).c_str());
  google::SetStderrLogging(google::GLOG_FATAL);
}

int main(int argc, char* argv[]) {
  dingodb::cli::ResetOptions();
  client_v2::CliState::GetInstance().Reset();
  BootstrapFormatFromArgv(argc, argv);
  InitLog("./log");

  RootCliOptions root_options;

  CLI::App app{kProgramDesc, kProgramName};
  app.get_formatter()->column_width(40);
  SetUpRootOptions(app, root_options);
  client_v2::SetUpCoordinatorSubCommands(app);
  client_v2::SetUpKVSubCommands(app);
  client_v2::SetUpMetaSubCommands(app);
  client_v2::SetUpStoreSubCommands(app);
  client_v2::SetUpDocumentIndexSubCommands(app);
  client_v2::SetUpToolSubCommands(app);
  client_v2::SetUpVectorIndexSubCommands(app);
  client_v2::SetUpRestoreSubCommands(app);

  auto* shell_cmd = app.add_subcommand("Shell", "Enter interactive CLI shell");
  shell_cmd->callback([&app]() {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
      client_v2::CliState::GetInstance().MarkUsageError("Shell command requires interactive TTY input/output.");
      return;
    }
    InteractiveCli(app);
  });

  if (argc <= 1) {
    std::cout << app.help() << std::endl;
    return 2;
  }

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    if (dingodb::cli::IsJsonOutput()) {
      if (e.get_exit_code() == 0) {
        return app.exit(e);
      }
      auto& state = client_v2::CliState::GetInstance();
      state.MarkUsageError(e.what());
      state.CaptureCommandNameFromApp(app);
      state.EmitJsonEnvelope();
      return 2;
    }
    const int code = app.exit(e);
    return code == 0 ? 0 : 2;
  } catch (const client_v2::CliExitException& e) {
    auto& state = client_v2::CliState::GetInstance();
    state.SetExitCode(e.code() == 0 ? 1 : e.code());
    std::string message = e.what();
    if (message.empty()) {
      message = "Command failed.";
    }
    if (!state.HasError()) {
      if (state.ExitCode() == 2) {
        state.MarkUsageError(message);
      } else {
        state.MarkRuntimeError(message);
      }
    }
    if (dingodb::cli::IsJsonOutput()) {
      state.CaptureCommandNameFromApp(app);
      state.EmitJsonEnvelope();
    }
    return state.ExitCode();
  } catch (const std::exception& e) {
    auto& state = client_v2::CliState::GetInstance();
    state.MarkRuntimeError(e.what());
    state.CaptureCommandNameFromApp(app);
    if (dingodb::cli::IsJsonOutput()) {
      state.EmitJsonEnvelope();
    }
    return state.ExitCode() == 0 ? 1 : state.ExitCode();
  }

  auto& state = client_v2::CliState::GetInstance();
  state.CaptureCommandNameFromApp(app);
  if (dingodb::cli::IsJsonOutput()) {
    state.EmitJsonEnvelope();
  }

  return state.ExitCode();
}
