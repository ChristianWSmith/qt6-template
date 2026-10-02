#pragma once

#include "../../../core/IModel.h"
#include "../../../core/IPersistenceProvider.h"
#include "../applogcommon.h"
#include <QObject>
#include <QString>
#include <QStringList>

class AppLogModel : public QObject, public IModel {
  Q_OBJECT

public:
  // Persistence of log messages demonstrates feature-state persistence via
  // IPersistenceProvider. It is not a recommendation that production
  // diagnostic logs be persisted as application state.

  explicit AppLogModel(IPersistenceProvider &provider,
                       QObject *parent = nullptr);

  // IModel
  PersistenceResult<void> saveState() const override;
  void loadState() override;

  void addLogMessage(const QString &message);
  void clear();
  [[nodiscard]] const QStringList &getLogMessages() const;

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();

private:
  IPersistenceProvider &m_provider;
  const QString m_key{APP_ID ".AppLogState"};

  QStringList m_logMessages;
};
