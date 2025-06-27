#pragma once

#include "../../../core/IModel.h"
#include "../../../core/IPersistenceProvider.h"
#include "../applogcommon.h"
#include <QMetaMethod>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtPlugin>

class AppLogModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit AppLogModel(IPersistenceProvider *provider = nullptr,
                       QObject *parent = nullptr);

  void shutdown() override;

  void addLogMessage(const QString &message);
  void clear();
  [[nodiscard]] const QVector<QString> &getLogMessages() const;

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();

private:
  friend class AppLogTest;

  IPersistenceProvider *m_provider;
  const QString m_key{APP_ID ".AppLogState"};

  void saveState() const override;
  void loadState() override;

  QVector<QString> m_logMessages;
};
