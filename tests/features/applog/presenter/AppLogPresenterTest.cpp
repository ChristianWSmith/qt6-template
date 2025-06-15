#include "features/applog/presenter/AppLogPresenter.h"
#include "events/LogEvent.h"
#include "features/applog/model/IAppLogModel.h"
#include "features/applog/widget/IAppLogWidget.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::NiceMock;

struct QCoreApplicationInitializer {
  QCoreApplicationInitializer() {
    if (!QCoreApplication::instance()) {
      static int argc = 0;
      new QCoreApplication(argc, nullptr);
    }
  };
};
static QCoreApplicationInitializer qappInitializer;

class MockAppLogModel : public IAppLogModel {
public:
  MOCK_METHOD(void, addLogMessage, (const QString &), (override));
  MOCK_METHOD(QMetaObject::Connection, connectLogMessageAdded,
              (QObject *, const char *), (override));
};

class MockAppLogWidget : public IAppLogWidget {
public:
  MOCK_METHOD(void, displayLogMessage, (const QString &), (override));
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
  EXPECT_CALL(*mockModel, connectLogMessageAdded(presenter, testing::_))
      .Times(1);
  delete presenter;
  presenter = new AppLogPresenter(mockModel, mockView);
}

TEST_F(AppLogPresenterTest, HandleLogMessageAddedForwardsToView) {
  QString testMessage = "Mocked log line";

  EXPECT_CALL(*mockView, displayLogMessage(testMessage)).Times(1);

  QMetaObject::invokeMethod(presenter, "handleLogMessageAdded",
                            Qt::DirectConnection, Q_ARG(QString, testMessage));
}

TEST_F(AppLogPresenterTest, OnLogEventReceivedForwardsToModel) {
  LogEvent event{"EventBus test message"};
  QString expected = QString::fromStdString(event.message);

  EXPECT_CALL(*mockModel, addLogMessage(expected)).Times(1);

  QMetaObject::invokeMethod(presenter, "onLogEventReceived",
                            Qt::DirectConnection, Q_ARG(LogEvent, event));
}
