#pragma once
#include <QJsonObject>
#include <QObject>
#include <cstdint>
#include <optional>
#include <variant>

enum class PersistenceError : std::uint8_t {
  NotFound,
  PermissionDenied,
  IoError,
  InvalidData,
  DurabilityFailure,
};

template <typename T> struct PersistenceResult {
  std::variant<T, PersistenceError> value;

  [[nodiscard]] bool hasValue() const {
    return std::holds_alternative<T>(value);
  }
  [[nodiscard]] bool hasError() const {
    return std::holds_alternative<PersistenceError>(value);
  }

  [[nodiscard]] const T &operator*() const { return std::get<T>(value); }
  T &operator*() { return std::get<T>(value); }

  [[nodiscard]] PersistenceError error() const {
    return std::get<PersistenceError>(value);
  }

  static PersistenceResult success(T result) {
    return PersistenceResult{std::move(result)};
  }
  static PersistenceResult failure(PersistenceError error) {
    return PersistenceResult{error};
  }

private:
  explicit PersistenceResult(std::variant<T, PersistenceError> val)
      : value(std::move(val)) {}
};

template <> struct PersistenceResult<void> {
  std::optional<PersistenceError> err;

  [[nodiscard]] bool hasValue() const { return !err.has_value(); }
  [[nodiscard]] bool hasError() const { return err.has_value(); }

  [[nodiscard]] PersistenceError error() const { return err.value(); }

  static PersistenceResult success() { return PersistenceResult{}; }
  static PersistenceResult failure(PersistenceError error) {
    return PersistenceResult{error};
  }

private:
  explicit PersistenceResult() = default;
  explicit PersistenceResult(PersistenceError error) : err(error) {}
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
