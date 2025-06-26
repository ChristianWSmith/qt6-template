// NOLINTBEGIN
#include "features/applog/presenter/AppLogPresenter.h"
#include "events/LogEvent.h"
#include "features/applog/applogcommon.h"
#include "features/applog/model/AppLogModel.h"
#include "features/applog/widget/AppLogWidget.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::NiceMock;

class MockAppLogModel : public AppLogModel {
public:
  MOCK_METHOD(void, addLogMessage, (const QString &), ());
  MOCK_METHOD(void, clear, (), ());
  MOCK_METHOD(const QVector<QString> &, getLogMessages, (), (const));
  MOCK_METHOD(void, shutdown, (), (override));

signals:
  void logChanged(const LogDelta &_t1);
  void logCleared();
};

class MockAppLogWidget : public AppLogWidget {
public:
  MOCK_METHOD(void, handleLogChanged, (const LogDelta &), ());
  MOCK_METHOD(void, shutdown, (), (override));
  MOCK_METHOD(void, clear, (), ());
  MOCK_METHOD(void, setLogMessages, (const QVector<QString> &), ());

signals:
  void clearRequested();
};

class AppLogPresenterTest : public ::testing::Test {
protected:
  NiceMock<MockAppLogModel> *mockModel;
  NiceMock<MockAppLogWidget> *mockView;
  AppLogPresenter *presenter;

  void SetUp() override {
    mockModel = new NiceMock<MockAppLogModel>();
    QVector<QString> dummyLogMessages{"line 1", "line 2"};
    ON_CALL(*mockModel, getLogMessages())
        .WillByDefault(::testing::ReturnRef(dummyLogMessages));

    mockView = new NiceMock<MockAppLogWidget>();
    presenter = new AppLogPresenter(mockModel, mockView);
  }

  void TearDown() override {
    delete presenter;
    delete mockModel;
    delete mockView;
  }
};

TEST_F(AppLogPresenterTest, HandleLogChangedForwardsToView) {
  QString testMessage = "Mocked log line";
  LogDelta logDelta = LogDelta{testMessage, false};

  EXPECT_CALL(*mockView, handleLogChanged(logDelta)).Times(1);
  QMetaObject::invokeMethod(presenter, "handleLogChanged", Qt::DirectConnection,
                            Q_ARG(const LogDelta &, logDelta));
}

TEST_F(AppLogPresenterTest, OnLogEventReceivedForwardsToModel) {
  LogEvent event{"EventBus test message"};
  QString expected = QString::fromStdString(event.message);

  EXPECT_CALL(*mockModel, addLogMessage(expected)).Times(1);

  QMetaObject::invokeMethod(presenter, "onLogEventReceived",
                            Qt::DirectConnection,
                            Q_ARG(const LogEvent &, event));
}
// NOLINTEND
