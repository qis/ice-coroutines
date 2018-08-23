#pragma once
#include <ice/async.hpp>
#include <functional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace ice {

enum class state {
  none,  // not handled
  more,  // handled, continue matching
  skip,  // handled, skip to end of line
  next,  // handled
  done,  // stop parser
};

using async_state = async<state>;

class parser {
public:
  using option = std::regex_constants::syntax_option_type;
  using handler = std::function<async_state(const std::smatch& sm, std::error_code& ec)>;

  void add(const std::string& regex, handler handler, option option = static_cast<option>(0))
  {
    matchers_.emplace_back(regex, std::move(handler), option);
  }

  async_state parse(char c, std::error_code& ec)
  {
    ec.clear();

    // Skip until the next '\n' after the current line was handled as a partial match.
    if (skip_) {
      if (c == '\n') {
        skip_ = false;
      }
      co_return state::none;
    }

    // Handle "\r\n" or '\n'.
    if (c == '\n') {
      if (line_.find_first_not_of(" \t\v\b\f\a") != std::string::npos) {
        lines_.push_back(std::move(line_));
      }
      line_.clear();
      cr_ = false;
      co_return state::none;
    }

    // Handle previously detected '\r' that is not followed by a '\n'.
    if (cr_) {
      if (line_.find_first_not_of(" \t\v\b\f\a") != std::string::npos) {
        lines_.push_back(std::move(line_));
      }
      line_.clear();
      cr_ = false;
    }

    // Detect '\r'.
    if (c == '\r') {
      cr_ = true;
      co_return state::none;
    }

    // Skip backspace.
    if (c == '\b') {
      co_return state::none;
    }

    line_.push_back(c);

    // Try to handle complete lines.
    for (auto it = lines_.begin(), end = lines_.end(); it != end; ++it) {
      if (co_await handle(*it, true, ec) == state::done || ec) {
        lines_.erase(lines_.begin(), ++it);
        co_return state::done;
      }
    }
    lines_.clear();

    // Resume if the current line is blank.
    if (line_.find_first_not_of(" \t\v\b\f\a") == std::string::npos) {
      co_return state::none;
    }

    // Try to handle a partial line.
    const auto state = co_await handle(line_, false, ec);
    if (state == state::done || ec) {
      co_return state::done;
    }
    switch (state) {
    case state::more:
    case state::done:
    case state::skip:
      skip_ = true;
      [[fallthrough]];
    case state::next:
      line_.clear();
      break;
    default:
      break;
    }
    co_return state;
  }

  template <typename Stream>
  ice::async<std::error_code> run(Stream& stream)
  {
    char c = '\0';
    std::error_code ec;
    while (!ec) {
      if (co_await stream.recv(&c, 1, ec) && co_await parse(c, ec) == state::done) {
        break;
      }
    }
    co_return ec;
  }

private:
  class matcher {
  public:
    matcher(const std::string& regex, parser::handler handler, option option) :
      regex_(regex, std::regex_constants::ECMAScript | option | std::regex_constants::optimize),
      handler_(std::move(handler))
    {}

    bool match(const std::string& string, std::smatch& sm, bool eol) const noexcept
    {
      const auto flags = eol ? std::regex_constants::match_not_null :
                               std::regex_constants::match_not_null | std::regex_constants::match_not_eol;
      return std::regex_match(string, sm, regex_, flags);
    }

    async_state handle(std::smatch& sm, std::error_code& ec) const
    {
      return handler_(sm, ec);
    }

  private:
    std::regex regex_;
    parser::handler handler_;
  };

  async_state handle(std::string& line, bool eol, std::error_code& ec) noexcept
  {
    std::smatch sm;
    state state = state::none;
    for (auto& matcher : matchers_) {
      if (matcher.match(line, sm, eol)) {
        state = co_await matcher.handle(sm, ec);
        if (state != state::more) {
          break;
        }
      }
    }
    co_return state;
  }

  bool cr_ = false;
  bool skip_ = false;

  std::string line_;
  std::vector<std::string> lines_;
  std::vector<matcher> matchers_;
};

}  // namespace ice
