#pragma once

#include "../../../core/IModel.h"
#include "../applogcommon.h"
#include <QMetaMethod>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtPlugin>

class AppLogModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit AppLogModel(QObject *parent = nullptr);
  void shutdown() override;

  void addLogMessage(const QString &message);
  void clear();
  [[nodiscard]] const QVector<QString> &getLogMessages() const;

  QMetaObject::Connection connectLogChanged(QObject *receiver,
                                            const char *member);
  QMetaObject::Connection connectLogCleared(QObject *receiver,
                                            const char *member);

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();

private:
  QVector<QString> m_logMessages;
  void saveState() const;
  void loadState();
};
