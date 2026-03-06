#ifndef DINGODB_COMMON_CLI_OPTIONS_H_
#define DINGODB_COMMON_CLI_OPTIONS_H_

#include <cstdint>
#include <string>

namespace dingodb::cli {

enum class OutputFormat : uint8_t {
  kAuto = 0,
  kTable = 1,
  kPlain = 2,
  kJson = 3,
};

struct Options {
  OutputFormat format{OutputFormat::kAuto};
  bool no_color{false};
  bool quiet{false};
  bool json_pretty{false};
  int64_t timeout_ms{60000};
  int max_retry{5};
};

Options& GetOptions();
void ResetOptions();

OutputFormat ParseOutputFormat(const std::string& value);
std::string OutputFormatToString(OutputFormat format);

inline bool IsJsonOutput() { return GetOptions().format == OutputFormat::kJson; }
inline bool IsPlainOutput() { return GetOptions().format == OutputFormat::kPlain; }

}  // namespace dingodb::cli

#endif  // DINGODB_COMMON_CLI_OPTIONS_H_
