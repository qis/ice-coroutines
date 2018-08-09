#pragma once
#include <ice/async.hpp>
#include <ice/log.hpp>
#include <functional>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace ice::state {

template <typename State>
class handler {
public:
  using regex_flag_type = std::regex::flag_type;
  using match_flag_type = std::regex_constants::match_flag_type;

  constexpr static regex_flag_type default_icase_flags(bool icase) noexcept
  {
    return icase ? std::regex_constants::icase : static_cast<regex_flag_type>(0);
  }

  constexpr static match_flag_type default_eol_flags(bool eol) noexcept
  {
    return eol ? static_cast<match_flag_type>(0) : std::regex_constants::match_not_eol;
  }

  template <typename Handler>
  handler(const std::string& regex, Handler&& handler, bool stop = true) :
    handler(regex, false, std::forward<Handler>(handler)), stop_(stop)
  {}

  template <typename Handler>
  handler(const std::string& regex, bool icase, Handler&& handler, bool stop = true) :
    regex_(regex, std::regex_constants::ECMAScript | default_icase_flags(icase) | std::regex_constants::optimize),
    handler_(std::forward<Handler>(handler)), stop_(stop)
  {}

  bool match(const std::string& string, std::smatch& sm, bool eol) const noexcept
  {
    return std::regex_match(string, sm, regex_, default_eol_flags(eol) | std::regex_constants::match_not_null);
  }

  ice::async<State> handle(std::smatch& sm, std::error_code& ec) const
  {
    ec.clear();
    co_return co_await handler_(sm, ec);
  }

  constexpr bool stop() const noexcept
  {
    return stop_;
  }

private:
  std::regex regex_;
  std::function<ice::async<State>(std::smatch&, std::error_code&)> handler_;
  bool stop_ = true;
};

template <typename State>
class manager {
public:
  template <typename... Args>
  manager(State state) : state_(state)
  {}

  template <typename Handler>
  void add(State state, const std::string& regex, Handler&& handler, bool stop = true)
  {
    add(state, regex, false, std::forward<Handler>(handler), stop);
  }

  template <typename Handler>
  void add(State state, const std::string& regex, bool icase, Handler&& handler, bool stop = true)
  {
    handlers_[state].emplace_back(regex, icase, std::forward<Handler>(handler), stop);
  }

  ice::async<std::error_code> parse(const char* data, std::size_t size) noexcept
  {
    // Append data to the current line and move the current line to a list of lines on '\n'.
    for (auto c : std::string_view{ data, size }) {
      // Skip until the next '\n' after the current line was handled as a partial match.
      if (skip_) {
        if (c == '\n') {
          skip_ = false;
        }
        continue;
      }

      // Handle "\r\n" or '\n'.
      if (c == '\n') {
        if (line_.find_first_not_of(" \t\v\b\f\a") != std::string::npos) {
          lines_.push_back(std::move(line_));
        }
        line_.clear();
        cr_ = false;
        continue;
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
        continue;
      }

      line_.push_back(c);
    }

    // Try to handle complete lines.
    for (const auto& e : lines_) {
      std::error_code ec;
      if (co_await handle(e, true, ec)) {
        if (ec) {
          co_return ec;
        }
      }
    }
    lines_.clear();

    // Resume if there was no timeout or the current line is blank.
    // NOTE: This does not work with streams that do not support timeouts for recv/send operations.
    //if (size || line_.find_first_not_of(" \t\v\b\f\a") == std::string::npos) {
    //  co_return{};
    //}

    // Resumt if the current line is blank.
    if (line_.find_first_not_of(" \t\v\b\f\a") == std::string::npos) {
      co_return{};
    }

    // Try to handle a partial line.
    std::error_code ec;
    if (co_await handle(line_, false, ec)) {
      if (ec) {
        co_return ec;
      }
      line_.clear();
      skip_ = true;
    }
    co_return{};
  }

  constexpr State state() const noexcept
  {
    return state_;
  }

  void state(State state) noexcept
  {
    state_ = state;
  }

protected:
  State state_;

private:
  ice::async<bool> handle(const std::string& line, bool eol, std::error_code& ec) noexcept
  {
    ec.clear();
    bool handled = false;
    if (const auto handlers = handlers_.find(state_); handlers != handlers_.end()) {
      if (!handlers->second.empty()) {
        for (const auto& e : handlers->second) {
          std::smatch sm;
          if (e.match(line, sm, eol)) {
            if (!std::exchange(handled, true) && line != last_log_entry_) {
              last_log_entry_ = line;
              ice::log::info(line);
            }
            state_ = co_await e.handle(sm, ec);
            if (e.stop()) {
              break;
            }
          }
        }
      } else {
        if (!std::exchange(state_warning_shown_, true)) {
          ice::log::warning("state without handlers: {}", static_cast<int>(state_));
        }
      }
    } else {
      if (!std::exchange(state_warning_shown_, true)) {
        ice::log::warning("state without entry: {}", static_cast<int>(state_));
      }
    }
    if (!handled && line != last_log_entry_) {
      last_log_entry_ = line;
      ice::log::debug(line);
    }
    co_return handled;
  }

  bool cr_ = false;
  bool skip_ = false;

  std::string line_;
  std::vector<std::string> lines_;
  std::string last_log_entry_;

  bool state_warning_shown_ = false;
  std::map<State, std::vector<handler<State>>> handlers_;
};

}  // namespace ice::state
