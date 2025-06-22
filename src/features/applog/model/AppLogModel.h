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

  QMetaObject::Connection connectLogChanged(QObject *receiver,
                                            const char *member) override;

signals:
  void logChanged(const LogDelta &_t1);

private:
  QVector<QString> m_logMessages;
};
