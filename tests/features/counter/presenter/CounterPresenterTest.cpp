// NOLINTBEGIN
#include "features/counter/presenter/CounterPresenter.h"
#include "features/counter/model/CounterModel.h"
#include "features/counter/widget/CounterWidget.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <QObject>
#include <qtmetamacros.h>

class MockCounterModel : public CounterModel {
public:
  MOCK_METHOD(void, increment, (), ());
  MOCK_METHOD(int, value, (), (const));
  MOCK_METHOD(void, reset, (), ());
  MOCK_METHOD(void, shutdown, (), (override));

signals:
  void valueChanged(int _t1);
};

class MockCounterWidget : public CounterWidget {
public:
  MOCK_METHOD(void, displayCounter, (int value), ());
  MOCK_METHOD(void, shutdown, (), (override));

signals:
  void incrementRequested();
  void resetRequested();
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

  EXPECT_CALL(*mockModel, value()).WillOnce(testing::Return(100));
  EXPECT_CALL(*mockView, displayCounter(100)).Times(1);

  presenter = new CounterPresenter(mockModel, mockView);
}

TEST_F(CounterPresenterTest, ConstructorHandlesNullModel) {

  EXPECT_CALL(*mockModel, value()).Times(0);

  EXPECT_CALL(*mockView, displayCounter(testing::_)).Times(0);

  presenter = new CounterPresenter(nullptr, mockView);
}

TEST_F(CounterPresenterTest, ConstructorHandlesNullView) {

  EXPECT_CALL(*mockView, displayCounter(testing::_)).Times(0);

  EXPECT_CALL(*mockModel, value()).Times(0);

  presenter = new CounterPresenter(mockModel, nullptr);
}

TEST_F(CounterPresenterTest,
       HandleIncrementRequestIncrementsModelAndPublishesLog) {

  EXPECT_CALL(*mockModel, value()).WillOnce(testing::Return(0));
  EXPECT_CALL(*mockView, displayCounter(0));
  presenter = new CounterPresenter(mockModel, mockView);

  EXPECT_CALL(*mockModel, increment()).Times(1);

  QMetaObject::invokeMethod(presenter, "handleIncrementRequest",
                            Qt::DirectConnection);
}

TEST_F(CounterPresenterTest,
       HandleCounterValueChangedUpdatesViewAndPublishesLog) {

  EXPECT_CALL(*mockModel, value()).WillOnce(testing::Return(0));
  EXPECT_CALL(*mockView, displayCounter(0));
  presenter = new CounterPresenter(mockModel, mockView);

  int newValue = 42;

  EXPECT_CALL(*mockView, displayCounter(newValue)).Times(1);

  QMetaObject::invokeMethod(presenter, "handleCounterValueChanged",
                            Qt::DirectConnection, Q_ARG(int, newValue));
}

TEST_F(CounterPresenterTest, HandleIncrementRequestNoOpWhenModelIsNull) {

  presenter = new CounterPresenter(nullptr, mockView);

  EXPECT_CALL(*mockModel, increment()).Times(0);

  QMetaObject::invokeMethod(presenter, "handleIncrementRequest",
                            Qt::DirectConnection);
}

TEST_F(CounterPresenterTest, HandleCounterValueChangedNoOpWhenViewIsNull) {

  EXPECT_CALL(*mockModel, value()).Times(0);

  presenter = new CounterPresenter(mockModel, nullptr);

  EXPECT_CALL(*mockView, displayCounter(testing::_)).Times(0);

  QMetaObject::invokeMethod(presenter, "handleCounterValueChanged",
                            Qt::DirectConnection, Q_ARG(int, 99));
}
// NOLINTEND
