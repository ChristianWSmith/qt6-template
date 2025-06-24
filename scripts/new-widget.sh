#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

WIDGET_NAME_TITLE="${1:-}"

usage() {
    echo "$(basename ${0}) <WidgetName>"
    echo "  - widget name should be a TitleCase valid file name"
    exit 1
}

format() {
    if [[ ! -z "$(which clang-format)" ]]; then
        clang-format -i "${1}"
    fi
}

if [[ "${WIDGET_NAME_TITLE}" == "" ]] || \
    [[ ! "${WIDGET_NAME_TITLE}" =~ ^[A-Z] ]] || \
    [[ ! "${WIDGET_NAME_TITLE}" =~ [A-Za-z]? ]]; then 
    usage
fi

WIDGET_NAME_UPPER="$(echo "${WIDGET_NAME_TITLE}" | tr '[:lower:]' '[:upper:]')"
WIDGET_NAME_LOWER="$(echo "${WIDGET_NAME_TITLE}" | tr '[:upper:]' '[:lower:]')"

WIDGET_DIR="${WIDGETS_DIR}/${WIDGET_NAME_LOWER}"

if [[ -e "${WIDGET_DIR}" ]]; then
  echo "Widget already exists."
  exit 1
fi

TEST_WIDGET_DIR="${TESTS_WIDGETS_DIR}/${WIDGET_NAME_LOWER}"

WIDGET_DIR_RELATIVE=${WIDGET_DIR#"${SRC_DIR}/"}

# COMMON
mkdir -p "${WIDGET_DIR}"
cat > "${WIDGET_DIR}/${WIDGET_NAME_LOWER}common.h" <<EOF
#pragma once
#include "../../core/interfacecommon.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ${WIDGET_NAME_UPPER}_WIDGET_ID APP_ID ".${WIDGET_NAME_TITLE}" QINTERFACE_WIDGET_SUFFIX

EOF
format "${WIDGET_DIR}/${WIDGET_NAME_LOWER}common.h"

# WIDGET INTERFACE
mkdir -p "${WIDGET_DIR}"
cat > "${WIDGET_DIR}/I${WIDGET_NAME_TITLE}Widget.h" <<EOF
#pragma once
#include "../../core/IWidget.h"
#include "../../core/interfacecommon.h"
#include "${WIDGET_NAME_LOWER}common.h"
#include <QWidget>
#include <QtPlugin>

class I${WIDGET_NAME_TITLE}Widget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit I${WIDGET_NAME_TITLE}Widget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~I${WIDGET_NAME_TITLE}Widget() = default;

  I${WIDGET_NAME_TITLE}Widget(const I${WIDGET_NAME_TITLE}Widget &) = delete;
  I${WIDGET_NAME_TITLE}Widget &operator=(const I${WIDGET_NAME_TITLE}Widget &) = delete;
  I${WIDGET_NAME_TITLE}Widget(I${WIDGET_NAME_TITLE}Widget &&) = delete;
  I${WIDGET_NAME_TITLE}Widget &operator=(I${WIDGET_NAME_TITLE}Widget &&) = delete;
  // Widget API methods and connections
};

Q_DECLARE_INTERFACE(I${WIDGET_NAME_TITLE}Widget,
                    ${WIDGET_NAME_UPPER}_WIDGET_ID)

EOF
format "${WIDGET_DIR}/I${WIDGET_NAME_TITLE}Widget.h"

# WIDGET HEADER
cat > "${WIDGET_DIR}/${WIDGET_NAME_TITLE}Widget.h" <<EOF
#pragma once
#include "I${WIDGET_NAME_TITLE}Widget.h"
#include "ui_${WIDGET_NAME_TITLE}Widget.h"

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ${WIDGET_NAME_TITLE}Widget : public I${WIDGET_NAME_TITLE}Widget {
  Q_OBJECT
  Q_INTERFACES(I${WIDGET_NAME_TITLE}Widget)

public:
  explicit ${WIDGET_NAME_TITLE}Widget(QWidget *parent = nullptr);
  ~${WIDGET_NAME_TITLE}Widget();

  ${WIDGET_NAME_TITLE}Widget(const ${WIDGET_NAME_TITLE}Widget &) = delete;
  ${WIDGET_NAME_TITLE}Widget &operator=(const ${WIDGET_NAME_TITLE}Widget &) = delete;
  ${WIDGET_NAME_TITLE}Widget(${WIDGET_NAME_TITLE}Widget &&) = delete;
  ${WIDGET_NAME_TITLE}Widget &operator=(${WIDGET_NAME_TITLE}Widget &&) = delete;

  void shutdown() override;

  // Implements I${WIDGET_NAME_TITLE}Widget methods

signals:
  // Signals emitted by this concrete Widget

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  Ui::${WIDGET_NAME_TITLE}Widget *ui;
};

EOF
format "${WIDGET_DIR}/${WIDGET_NAME_TITLE}Widget.h"

# WIDGET CPP
cat > "${WIDGET_DIR}/${WIDGET_NAME_TITLE}Widget.cpp" <<EOF
#include "${WIDGET_NAME_TITLE}Widget.h"

${WIDGET_NAME_TITLE}Widget::${WIDGET_NAME_TITLE}Widget(QWidget *parent)
    : I${WIDGET_NAME_TITLE}Widget(parent), ui(new Ui::${WIDGET_NAME_TITLE}Widget) {
  ui->setupUi(this);
}

${WIDGET_NAME_TITLE}Widget::~${WIDGET_NAME_TITLE}Widget() { delete ui; }

// Implements I${WIDGET_NAME_TITLE}Widget methods and internal connections

// Implements UI slots, typically emitting signals to the Presenter

void ${WIDGET_NAME_TITLE}Widget::shutdown() {}

EOF
format "${WIDGET_DIR}/${WIDGET_NAME_TITLE}Widget.cpp"

# WIDGET UI
cat > "${WIDGET_DIR}/${WIDGET_NAME_TITLE}Widget.ui" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>${WIDGET_NAME_TITLE}Widget</class>
 <widget class="QWidget" name="${WIDGET_NAME_TITLE}Widget">
  <property name="geometry">
   <rect>
    <x>0</x>
    <y>0</y>
    <width>12</width>
    <height>12</height>
   </rect>
  </property>
  <layout class="QVBoxLayout" name="verticalLayout"/>
 </widget>
 <resources/>
 <connections/>
</ui>

EOF

# WIDGET TEST
mkdir -p "${TEST_WIDGET_DIR}"
cat > "${TEST_WIDGET_DIR}/${WIDGET_NAME_TITLE}WidgetTest.cpp" <<EOF
// NOLINTBEGIN
#include <gtest/gtest.h>
#include "${WIDGET_DIR_RELATIVE}/${WIDGET_NAME_TITLE}Widget.h"

class ${WIDGET_NAME_TITLE}WidgetTest : public ::testing::Test {
protected:
  ${WIDGET_NAME_TITLE}Widget *widget = nullptr;

  void SetUp() override {
    widget = new ${WIDGET_NAME_TITLE}Widget();
    widget->show();
  }

  void TearDown() override { 
    widget->hide();
    delete widget; 
  }
};

// Add your widget tests here
TEST_F(${WIDGET_NAME_TITLE}WidgetTest, Placeholder) {
  EXPECT_TRUE(true);
}
// NOLINTEND

EOF
format "${TEST_WIDGET_DIR}/${WIDGET_NAME_TITLE}WidgetTest.cpp"
