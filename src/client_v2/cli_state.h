#ifndef DINGODB_CLIENT_V2_CLI_STATE_H_
#define DINGODB_CLIENT_V2_CLI_STATE_H_

#include <chrono>
#include <exception>
#include <memory>
#include <string>

#include "CLI/CLI.hpp"
#include "google/protobuf/message.h"
#include "nlohmann/json.hpp"

namespace client_v2 {

class CliExitException : public std::exception {
 public:
  CliExitException(int code, std::string message) : code_(code), message_(std::move(message)) {}
  ~CliExitException() override = default;

  const char* what() const noexcept override;
  int code() const noexcept { return code_; }

 private:
  int code_;
  std::string message_;
};

[[noreturn]] void ThrowCliExit(int code, const std::string& message = "");

class CliState {
 public:
  static CliState& GetInstance();

  void Reset();

  int ExitCode() const { return exit_code_; }
  void SetExitCode(int exit_code);

  bool HasError() const { return has_error_; }
  bool HasJsonData() const;

  void SetCommandName(const std::string& command_name);
  void CaptureCommandNameFromApp(const CLI::App& app);
  const std::string& CommandName() const { return command_name_; }

  void MarkUsageError(const std::string& message, int code = 0, const std::string& name = "");
  void MarkRuntimeError(const std::string& message, int code = 0, const std::string& name = "");

  void SetJsonData(const nlohmann::json& data);
  void SetJsonDataFromProto(const google::protobuf::Message& message);

  void EmitJsonEnvelope() const;

 private:
  CliState() = default;

  void SetError(const std::string& kind, int exit_code, const std::string& message, int code, const std::string& name);
  int64_t ElapsedMs() const;

  int exit_code_{0};
  bool has_error_{false};
  std::string command_name_;
  std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};

  std::unique_ptr<nlohmann::json> data_json_;
  std::unique_ptr<nlohmann::json> error_json_;
};

}  // namespace client_v2

#endif  // DINGODB_CLIENT_V2_CLI_STATE_H_
