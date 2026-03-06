#include "common/cli_options.h"

#include <algorithm>
#include <cctype>

namespace dingodb::cli {

Options& GetOptions() {
  static Options options;
  return options;
}

void ResetOptions() { GetOptions() = Options{}; }

OutputFormat ParseOutputFormat(const std::string& value) {
  std::string normalized = value;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

  if (normalized == "table") {
    return OutputFormat::kTable;
  }
  if (normalized == "plain") {
    return OutputFormat::kPlain;
  }
  if (normalized == "json") {
    return OutputFormat::kJson;
  }
  return OutputFormat::kAuto;
}

std::string OutputFormatToString(OutputFormat format) {
  switch (format) {
    case OutputFormat::kAuto:
      return "auto";
    case OutputFormat::kTable:
      return "table";
    case OutputFormat::kPlain:
      return "plain";
    case OutputFormat::kJson:
      return "json";
  }

  return "auto";
}

}  // namespace dingodb::cli
