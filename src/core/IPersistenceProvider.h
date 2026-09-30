#pragma once
#include <QJsonObject>
#include <QObject>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <variant>

enum class PersistenceError : std::uint8_t {
  NotFound,
  IoError,
  InvalidData,
  DurabilityFailure,
};

constexpr std::string_view toString(PersistenceError error) {
  switch (error) {
  case PersistenceError::NotFound: return "NotFound";
  case PersistenceError::IoError: return "IoError";
  case PersistenceError::InvalidData: return "InvalidData";
  case PersistenceError::DurabilityFailure: return "DurabilityFailure";
  }
  return "Unknown";
}

/// Explicit load/save result under C++20 (std::expected is C++23).
///
/// Contract:
///   - Inspect hasValue()/hasError() before calling value() or error().
///   - Misuse (value() on error, error() on success) is a programming error:
///     asserted in debug builds, aborts in release. No exceptions; no
///     fabricated values.
///   - Storage is private; use the accessors only.
template <typename T> class PersistenceResult {
public:
  [[nodiscard]] bool hasValue() const {
    return std::holds_alternative<T>(storage_);
  }
  [[nodiscard]] bool hasError() const {
    return std::holds_alternative<PersistenceError>(storage_);
  }

  [[nodiscard]] const T &value() const {
    assert(hasValue() && "PersistenceResult::value() on error result");
    if (!hasValue()) {
      std::abort();
    }
    return std::get<T>(storage_);
  }

  [[nodiscard]] T &value() {
    assert(hasValue() && "PersistenceResult::value() on error result");
    if (!hasValue()) {
      std::abort();
    }
    return std::get<T>(storage_);
  }

  [[nodiscard]] PersistenceError error() const {
    assert(hasError() && "PersistenceResult::error() on success result");
    if (!hasError()) {
      std::abort();
    }
    return std::get<PersistenceError>(storage_);
  }

  static PersistenceResult success(T result) {
    return PersistenceResult{std::move(result)};
  }
  static PersistenceResult failure(PersistenceError error) {
    return PersistenceResult{error};
  }

private:
  explicit PersistenceResult(std::variant<T, PersistenceError> val)
      : storage_(std::move(val)) {}

  std::variant<T, PersistenceError> storage_;
};

template <> class PersistenceResult<void> {
public:
  [[nodiscard]] bool hasValue() const { return !err_.has_value(); }
  [[nodiscard]] bool hasError() const { return err_.has_value(); }

  [[nodiscard]] PersistenceError error() const {
    assert(hasError() && "PersistenceResult::error() on success result");
    if (!err_.has_value()) {
      std::abort();
    }
    return *err_;
  }

  static PersistenceResult success() { return PersistenceResult{}; }
  static PersistenceResult failure(PersistenceError error) {
    return PersistenceResult{error};
  }

private:
  explicit PersistenceResult() = default;
  explicit PersistenceResult(PersistenceError error) : err_(error) {}

  std::optional<PersistenceError> err_;
};

class IPersistenceProvider {
public:
  IPersistenceProvider() = default;
  virtual ~IPersistenceProvider() = default;

  IPersistenceProvider(const IPersistenceProvider &) = delete;
  IPersistenceProvider &operator=(const IPersistenceProvider &) = delete;
  IPersistenceProvider(IPersistenceProvider &&) = delete;
  IPersistenceProvider &operator=(IPersistenceProvider &&) = delete;

  virtual PersistenceResult<QJsonObject> loadState(const QString &key) = 0;
  virtual PersistenceResult<void> saveState(const QString &key,
                                             const QJsonObject &state) = 0;
};
