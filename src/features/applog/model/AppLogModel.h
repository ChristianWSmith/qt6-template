#ifndef APPLOGMODEL_H
#define APPLOGMODEL_H

#include "IAppLogModel.h"
#include <QString>
#include <QVector>

class AppLogModel : public IAppLogModel {
  Q_OBJECT
  Q_INTERFACES(IAppLogModel)

public:
  explicit AppLogModel(QObject *parent = nullptr);

  void addLogMessage(const QString &message) override;

  QMetaObject::Connection connectLogMessageAdded(QObject *receiver,
                                                 const char *member) override;

signals:
  void logMessageAdded(const QString &message);

private:
  QVector<QString> m_logMessages;
  const int m_maxLogMessages = 500;
};

#endif
