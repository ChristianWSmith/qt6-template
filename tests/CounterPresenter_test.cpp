#include "features/counter/model/ICounterModel.h"
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/widget/ICounterWidget.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <QDebug>
#include <QMetaObject>
#include <QObject>

class MockCounterModel;
class MockCounterWidget;

class MockCounterModel : public ICounterModel {
public:
  MOCK_METHOD(void, increment, (), (override));
  MOCK_METHOD(int, value, (), (const, override));
  MOCK_METHOD(QMetaObject::Connection, connectValueChanged,
              (QObject * receiver, const char *member), (override));
};

class MockCounterWidget : public ICounterWidget {
public:
  MOCK_METHOD(void, displayCounter, (int value), (override));
  MOCK_METHOD(QMetaObject::Connection, connectIncrementRequested,
              (QObject * receiver, const char *member), (override));
};

class CounterPresenterTest : public ::testing::Test {
protected:
  MockCounterModel *mockModel;
  MockCounterWidget *mockView;
  CounterPresenter *presenter;

  void SetUp() override {

    mockModel = new MockCounterModel();
    mockView = new MockCounterWidget();
  }

  void TearDown() override {

    delete presenter;
    delete mockModel;
    delete mockView;
  }
};

TEST_F(CounterPresenterTest,
       ConstructorEstablishesConnectionsAndDisplaysInitialValue) {

  EXPECT_CALL(*mockView, connectIncrementRequested(
                             testing::A<QObject *>(),
                             testing::StrEq(SLOT(handleIncrementRequest()))))
      .Times(1);
  EXPECT_CALL(
      *mockModel,
      connectValueChanged(testing::A<QObject *>(),
                          testing::StrEq(SLOT(handleCounterValueChanged(int)))))
      .Times(1);

  EXPECT_CALL(*mockModel, value()).WillOnce(testing::Return(100));
  EXPECT_CALL(*mockView, displayCounter(100)).Times(1);

  presenter = new CounterPresenter(mockModel, mockView);
}

TEST_F(CounterPresenterTest, ConstructorHandlesNullModel) {

  EXPECT_CALL(*mockModel, connectValueChanged(testing::_, testing::_)).Times(0);
  EXPECT_CALL(*mockModel, value()).Times(0);

  EXPECT_CALL(*mockView, connectIncrementRequested(
                             testing::A<QObject *>(),
                             testing::StrEq(SLOT(handleIncrementRequest()))))
      .Times(1);
  EXPECT_CALL(*mockView, displayCounter(testing::_)).Times(0);

  presenter = new CounterPresenter(nullptr, mockView);
}

TEST_F(CounterPresenterTest, ConstructorHandlesNullView) {

  EXPECT_CALL(*mockView, connectIncrementRequested(testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(*mockView, displayCounter(testing::_)).Times(0);

  EXPECT_CALL(
      *mockModel,
      connectValueChanged(testing::A<QObject *>(),
                          testing::StrEq(SLOT(handleCounterValueChanged(int)))))
      .Times(1);
  EXPECT_CALL(*mockModel, value()).Times(0);

  presenter = new CounterPresenter(mockModel, nullptr);
}

TEST_F(CounterPresenterTest,
       HandleIncrementRequestIncrementsModelAndPublishesLog) {

  EXPECT_CALL(*mockView, connectIncrementRequested(testing::_, testing::_));
  EXPECT_CALL(*mockModel, connectValueChanged(testing::_, testing::_));
  EXPECT_CALL(*mockModel, value()).WillOnce(testing::Return(0));
  EXPECT_CALL(*mockView, displayCounter(0));
  presenter = new CounterPresenter(mockModel, mockView);

  EXPECT_CALL(*mockModel, increment()).Times(1);

  QMetaObject::invokeMethod(presenter, "handleIncrementRequest",
                            Qt::DirectConnection);
}

TEST_F(CounterPresenterTest,
       HandleCounterValueChangedUpdatesViewAndPublishesLog) {

  EXPECT_CALL(*mockView, connectIncrementRequested(testing::_, testing::_));
  EXPECT_CALL(*mockModel, connectValueChanged(testing::_, testing::_));
  EXPECT_CALL(*mockModel, value()).WillOnce(testing::Return(0));
  EXPECT_CALL(*mockView, displayCounter(0));
  presenter = new CounterPresenter(mockModel, mockView);

  int newValue = 42;

  EXPECT_CALL(*mockView, displayCounter(newValue)).Times(1);

  QMetaObject::invokeMethod(presenter, "handleCounterValueChanged",
                            Qt::DirectConnection, Q_ARG(int, newValue));
}

TEST_F(CounterPresenterTest, HandleIncrementRequestNoOpWhenModelIsNull) {

  EXPECT_CALL(*mockView, connectIncrementRequested(testing::_, testing::_));
  presenter = new CounterPresenter(nullptr, mockView);

  EXPECT_CALL(*mockModel, increment()).Times(0);

  QMetaObject::invokeMethod(presenter, "handleIncrementRequest",
                            Qt::DirectConnection);
}

TEST_F(CounterPresenterTest, HandleCounterValueChangedNoOpWhenViewIsNull) {

  EXPECT_CALL(*mockModel, connectValueChanged(testing::_, testing::_));

  EXPECT_CALL(*mockModel, value()).Times(0);

  presenter = new CounterPresenter(mockModel, nullptr);

  EXPECT_CALL(*mockView, displayCounter(testing::_)).Times(0);

  QMetaObject::invokeMethod(presenter, "handleCounterValueChanged",
                            Qt::DirectConnection, Q_ARG(int, 99));
}
