#pragma once

#include "../../../core/IModel.h"
#include "../../../core/IPersistenceProvider.h"
#include "../applogcommon.h"
#include <QObject>
#include <QString>
#include <QVector>

class AppLogModel : public QObject, public IModel {
  Q_OBJECT

public:
  // Persistence of log messages demonstrates feature-state persistence via
  // IPPersistenceProvider. It is not a recommendation that production
  // diagnostic logs be persisted as application state (see AGENTS.md).

  explicit AppLogModel(IPersistenceProvider &provider,
                       QObject *parent = nullptr);

  // IModel
  PersistenceResult<void> saveState() const override;
  void loadState() override;

  void addLogMessage(const QString &message);
  void clear();
  [[nodiscard]] const QVector<QString> &getLogMessages() const;

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();

private:
  friend class AppLogTest;

  IPersistenceProvider &m_provider;
  const QString m_key{APP_ID ".AppLogState"};

  QVector<QString> m_logMessages;
};
