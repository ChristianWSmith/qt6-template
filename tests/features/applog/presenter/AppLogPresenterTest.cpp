// NOLINTBEGIN
#include "features/applog/presenter/AppLogPresenter.h"
#include "events/LogEvent.h"
#include "features/applog/applogcommon.h"
#include "features/applog/model/IAppLogModel.h"
#include "features/applog/widget/IAppLogWidget.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::NiceMock;

class MockAppLogModel : public IAppLogModel {
public:
  MOCK_METHOD(void, addLogMessage, (const QString &), (override));
  MOCK_METHOD(QMetaObject::Connection, connectLogChanged,
              (QObject *, const char *), (override));
  MOCK_METHOD(void, shutdown, (), (override));
};

class MockAppLogWidget : public IAppLogWidget {
public:
  MOCK_METHOD(void, handleLogChanged, (const LogDelta &), (override));
  MOCK_METHOD(void, shutdown, (), (override));
};

class AppLogPresenterTest : public ::testing::Test {
protected:
  NiceMock<MockAppLogModel> *mockModel;
  NiceMock<MockAppLogWidget> *mockView;
  AppLogPresenter *presenter;

  void SetUp() override {
    mockModel = new NiceMock<MockAppLogModel>();
    mockView = new NiceMock<MockAppLogWidget>();
    presenter = new AppLogPresenter(mockModel, mockView);
  }

  void TearDown() override {
    delete presenter;
    delete mockModel;
    delete mockView;
  }
};

TEST_F(AppLogPresenterTest, ConstructorConnectsModelSignal) {
  EXPECT_CALL(*mockModel, connectLogChanged(testing::_, testing::_))
      .WillOnce([](QObject *receiver, const char *signal) {
        std::cerr << "connectLogChanged called with signal: " << signal << "\n";
        std::string actual(signal);
        EXPECT_EQ(actual, "1handleLogChanged(LogDelta)");
        return QMetaObject::Connection();
      });

  delete presenter;
  presenter = new AppLogPresenter(mockModel, mockView);
}

TEST_F(AppLogPresenterTest, HandleLogChangedForwardsToView) {
  QString testMessage = "Mocked log line";
  const LogDelta logDelta = LogDelta{testMessage, false};

  EXPECT_CALL(*mockView, handleLogChanged(logDelta)).Times(1);

  QMetaObject::invokeMethod(presenter, "handleLogChanged", Qt::DirectConnection,
                            Q_ARG(LogDelta, logDelta));
}

TEST_F(AppLogPresenterTest, OnLogEventReceivedForwardsToModel) {
  LogEvent event{"EventBus test message"};
  QString expected = QString::fromStdString(event.message);

  EXPECT_CALL(*mockModel, addLogMessage(expected)).Times(1);

  QMetaObject::invokeMethod(presenter, "onLogEventReceived",
                            Qt::DirectConnection, Q_ARG(LogEvent, event));
}
// NOLINTEND
