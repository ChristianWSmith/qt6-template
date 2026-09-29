#pragma once

/// Models own application state and feature behavior.
///
/// Lifecycle:
///   - Constructors may call loadState() to restore persisted state.
///   - Missing state (NotFound) is a normal first-run condition.
///   - Other persistence errors are logged but do not prevent operation.
///   - saveState() should be called when state needs to persist.
///   - saveState() is synchronous and durable before returning.
///   - All methods execute on the GUI thread.
class IModel {
public:
  IModel() = default;
  virtual ~IModel() = default;

  IModel(const IModel &) = delete;
  IModel &operator=(const IModel &) = delete;
  IModel(IModel &&) = delete;
  IModel &operator=(IModel &&) = delete;

  virtual void loadState() = 0;
  virtual void saveState() const = 0;
};
