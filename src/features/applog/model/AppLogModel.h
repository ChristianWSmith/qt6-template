#pragma once

#include "IAppLogModel.h"
#include <QString>
#include <QVector>

class AppLogModel : public IAppLogModel {
  Q_OBJECT
  Q_INTERFACES(IAppLogModel)

public:
  explicit AppLogModel(QObject *parent = nullptr);
  void shutdown() override;

  void addLogMessage(const QString &message) override;
  void clear() override;
  [[nodiscard]] const QVector<QString> &getLogMessages() const override;

  QMetaObject::Connection connectLogChanged(QObject *receiver,
                                            const char *member) override;
  QMetaObject::Connection connectLogCleared(QObject *receiver,
                                            const char *member) override;

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();

private:
  QVector<QString> m_logMessages;
  void saveState() const;
  void loadState();
};
