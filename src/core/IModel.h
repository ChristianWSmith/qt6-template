#pragma once
#include "IPersistenceProvider.h"

/// Models own application state and feature behavior.
///
/// Lifecycle:
///   - Constructors may call loadState() to restore persisted state.
///   - Missing state (NotFound) is a normal first-run condition.
///   - The persistence provider logs operational persistence failures.
///     Models treat NotFound as first-run state and otherwise branch on the
///     result without emitting duplicate persistence diagnostics.
///   - saveState() should be called when state needs to persist.
///   - saveState() returns the provider's PersistenceResult so callers
///     (e.g. the composition root) can observe outcomes. Models forward
///     the provider result and do not log errors at the model layer.
///   - API asymmetry is intentional today: loadState() returns void
///     (models treat load errors internally — NotFound is first-run;
///     operational errors are logged by the provider) while saveState()
///     returns PersistenceResult<void> so the composition root can observe
///     outcomes. Do not invent a parallel load-result API.
///   - saveState() is synchronous. FilePersistenceProvider commits via
///     QSaveFile atomic replace (not a power-loss durability guarantee).
///   - All methods execute on the GUI thread.
///   - Persistence paths use QStandardPaths::AppDataLocation; tests enable
///     Qt test mode (tests/main.cpp) so runs never touch real app data.
class IModel {
public:
  IModel() = default;
  virtual ~IModel() = default;

  IModel(const IModel &) = delete;
  IModel &operator=(const IModel &) = delete;
  IModel(IModel &&) = delete;
  IModel &operator=(IModel &&) = delete;

  /// Absorbs load failures at the model boundary by design (see class docs).
  virtual void loadState() = 0;
  /// Discarding this result silently drops typed save errors — always observe.
  [[nodiscard]] virtual PersistenceResult<void> saveState() const = 0;
};
