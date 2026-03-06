#include "client_v2/cli_state.h"

#include <cstdint>
#include <iostream>

#include "common/cli_options.h"
#include "google/protobuf/util/json_util.h"
#include "nlohmann/json.hpp"

namespace client_v2 {

namespace {

const CLI::App* FindParsedSubcommand(const CLI::App& app) {
  for (const auto* subcommand : app.get_subcommands()) {
    if (!subcommand->parsed()) {
      continue;
    }

    if (const auto* nested = FindParsedSubcommand(*subcommand); nested != nullptr) {
      return nested;
    }

    return subcommand;
  }

  return nullptr;
}

}  // namespace

const char* CliExitException::what() const noexcept {
  return message_.c_str();
}

[[noreturn]] void ThrowCliExit(int code, const std::string& message) { throw CliExitException(code, message); }

CliState& CliState::GetInstance() {
  static CliState state;
  return state;
}

void CliState::Reset() {
  exit_code_ = 0;
  has_error_ = false;
  command_name_.clear();
  start_time_ = std::chrono::steady_clock::now();
  data_json_.reset();
  error_json_.reset();
}

void CliState::SetExitCode(int exit_code) {
  if (exit_code_ == 0) {
    exit_code_ = exit_code;
    return;
  }

  if (exit_code == 2 && exit_code_ != 2) {
    exit_code_ = 2;
  }
}

bool CliState::HasJsonData() const { return data_json_ != nullptr; }

void CliState::SetCommandName(const std::string& command_name) { command_name_ = command_name; }

void CliState::CaptureCommandNameFromApp(const CLI::App& app) {
  const auto* subcommand = FindParsedSubcommand(app);
  if (subcommand == nullptr) {
    if (!app.get_name().empty()) {
      command_name_ = app.get_name();
    }
    return;
  }

  command_name_ = subcommand->get_name();
}

void CliState::SetError(const std::string& kind, int exit_code, const std::string& message, int code,
                        const std::string& name) {
  has_error_ = true;
  SetExitCode(exit_code);

  error_json_ = std::make_unique<nlohmann::json>();
  (*error_json_)["kind"] = kind;
  (*error_json_)["code"] = code;
  (*error_json_)["name"] = name;
  (*error_json_)["message"] = message;
}

void CliState::MarkUsageError(const std::string& message, int code, const std::string& name) {
  SetError("usage", 2, message, code, name);
  if (!dingodb::cli::IsJsonOutput()) {
    std::cerr << "Error: " << message << std::endl;
  }
}

void CliState::MarkRuntimeError(const std::string& message, int code, const std::string& name) {
  SetError("runtime", 1, message, code, name);
  if (!dingodb::cli::IsJsonOutput()) {
    std::cerr << "Error: " << message << std::endl;
  }
}

void CliState::SetJsonData(const nlohmann::json& data) {
  if (!dingodb::cli::IsJsonOutput()) {
    return;
  }

  data_json_ = std::make_unique<nlohmann::json>(data);
}

void CliState::SetJsonDataFromProto(const google::protobuf::Message& message) {
  if (!dingodb::cli::IsJsonOutput()) {
    return;
  }

  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = dingodb::cli::GetOptions().json_pretty;
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;

  std::string json_string;
  auto status = google::protobuf::util::MessageToJsonString(message, &json_string, options);
  if (!status.ok()) {
    MarkRuntimeError("Failed to serialize protobuf message to JSON.");
    return;
  }

  if (!nlohmann::json::accept(json_string)) {
    MarkRuntimeError("Failed to parse protobuf JSON payload.");
    return;
  }

  data_json_ = std::make_unique<nlohmann::json>(nlohmann::json::parse(json_string));
}

int64_t CliState::ElapsedMs() const {
  const auto duration = std::chrono::steady_clock::now() - start_time_;
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void CliState::EmitJsonEnvelope() const {
  if (!dingodb::cli::IsJsonOutput()) {
    return;
  }

  nlohmann::json envelope = nlohmann::json::object();
  envelope["ok"] = !has_error_;
  envelope["command"] = command_name_;
  envelope["meta"] = {
      {"elapsed_ms", ElapsedMs()},
      {"format", "json"},
  };

  if (!has_error_) {
    if (data_json_ != nullptr) {
      envelope["data"] = *data_json_;
    } else {
      envelope["data"] = nlohmann::json::object();
    }
  } else {
    if (error_json_ != nullptr) {
      envelope["error"] = *error_json_;
    } else {
      envelope["error"] = {
          {"kind", "runtime"},
          {"code", 0},
          {"name", ""},
          {"message", "unknown error"},
      };
    }
  }

  const int indent = dingodb::cli::GetOptions().json_pretty ? 2 : -1;
  if (has_error_) {
    std::cerr << envelope.dump(indent) << std::endl;
  } else {
    std::cout << envelope.dump(indent) << std::endl;
  }
}

}  // namespace client_v2
