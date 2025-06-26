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

  virtual void shutdown() override;

  virtual void addLogMessage(const QString &message);
  virtual void clear();
  [[nodiscard]] virtual const QVector<QString> &getLogMessages() const;

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();

private:
  QVector<QString> m_logMessages;
  void saveState() const;
  void loadState();
};
