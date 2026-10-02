#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/env.sh"

# gtest_discover_tests POST_BUILD runs UnitTests; need Qt on PATH and a
# headless platform plugin (same requirements as scripts/build.sh --test).
export PATH="${QT_BIN}:${PATH}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

NAME_TITLE="GenSmokeProbe"
NAME_LOWER="$(echo "${NAME_TITLE}" | tr '[:upper:]' '[:lower:]')"
FEATURE_DIR="${FEATURES_DIR}/${NAME_LOWER}"
TEST_FILE="${TESTS_FEATURES_DIR}/${NAME_TITLE}Test.cpp"

MODEL_H="${FEATURE_DIR}/model/${NAME_TITLE}Model.h"
MODEL_CPP="${FEATURE_DIR}/model/${NAME_TITLE}Model.cpp"
PRESENTER_CPP="${FEATURE_DIR}/presenter/${NAME_TITLE}Presenter.cpp"

cleanup() {
  rm -rf "${FEATURE_DIR}"
  rm -f "${TEST_FILE}"
}

if [[ -e "${FEATURE_DIR}" ]] || [[ -e "${TEST_FILE}" ]]; then
  echo "Smoke probe ${NAME_TITLE} already exists; refusing to overwrite."
  exit 1
fi

trap cleanup EXIT

echo "==> Generating feature ${NAME_TITLE}"
"${SCRIPT_DIR}/generate.sh" feature "${NAME_TITLE}"

FAILED=0

check() {
  local desc="$1"
  local file="$2"
  local pattern="$3"
  local invert="${4:-}"

  if [[ ! -f "${file}" ]]; then
    echo "FAIL: ${desc} (missing file: ${file})"
    FAILED=1
    return
  fi

  if [[ -n "${invert}" ]]; then
    if grep -qE "${pattern}" "${file}"; then
      echo "FAIL: ${desc} (forbidden pattern found: ${pattern})"
      FAILED=1
    else
      echo "PASS: ${desc}"
    fi
  else
    if grep -qE "${pattern}" "${file}"; then
      echo "PASS: ${desc}"
    else
      echo "FAIL: ${desc} (missing pattern: ${pattern})"
      FAILED=1
    fi
  fi
}

echo "==> Running convention checks"

check "Model.h uses IPersistenceProvider reference" \
  "${MODEL_H}" 'IPersistenceProvider &provider'

check "Model.h does not default provider to nullptr" \
  "${MODEL_H}" 'provider = nullptr' invert

check "Model.h does not take provider by pointer" \
  "${MODEL_H}" 'IPersistenceProvider \*provider' invert

check "Model.h stores provider by reference" \
  "${MODEL_H}" 'IPersistenceProvider &m_provider'

check "Model.cpp uses qCDebug category logging" \
  "${MODEL_CPP}" 'qCDebug\(appPersistence\)'

check "Model.cpp saveState forwards to provider (not unconditional success)" \
  "${MODEL_CPP}" 'return m_provider\.saveState\(m_key, obj\)'

check "Model.cpp does not return bare PersistenceResult success in saveState" \
  "${MODEL_CPP}" 'return PersistenceResult<void>::success\(\);' invert

check "Model.cpp does not null-guard provider" \
  "${MODEL_CPP}" 'm_provider == nullptr' invert

check "Model.cpp loadState scaffold mentions provider loadState" \
  "${MODEL_CPP}" 'm_provider\.loadState'

check "Presenter.cpp uses Q_ASSERT for model" \
  "${PRESENTER_CPP}" 'Q_ASSERT\(m_model != nullptr\)'

check "Presenter.cpp uses Q_ASSERT for view" \
  "${PRESENTER_CPP}" 'Q_ASSERT\(m_view != nullptr\)'

check "Presenter.cpp scaffolds initial model→view sync after connects" \
  "${PRESENTER_CPP}" 'Initial model → view synchronization'

check "Presenter.cpp does not declare a destructor" \
  "${PRESENTER_CPP}" '~[A-Za-z0-9_]*Presenter' invert

check "Presenter.h does not declare friend test class" \
  "${FEATURE_DIR}/presenter/${NAME_TITLE}Presenter.h" 'friend class' invert

check "Model.h does not declare friend test class" \
  "${MODEL_H}" 'friend class' invert

check "Widget.h does not declare friend test class" \
  "${FEATURE_DIR}/widget/${NAME_TITLE}Widget.h" 'friend class' invert

check "common.h documents shared-type purpose" \
  "${FEATURE_DIR}/${NAME_LOWER}common.h" 'shared header|shared types|Feature-local'

check "common.h does not declare a Test class forward declaration" \
  "${FEATURE_DIR}/${NAME_LOWER}common.h" 'class .*Test;' invert

check "Test fixture uses MemoryPersistenceProvider" \
  "${TEST_FILE}" 'MemoryPersistenceProvider provider;'

check "Test fixture wires model(provider, nullptr)" \
  "${TEST_FILE}" 'model\(provider, nullptr\)'

check "Test fixture wires presenter(&model, &view)" \
  "${TEST_FILE}" 'presenter\(&model, &view\)'

check "Test file includes .moc" \
  "${TEST_FILE}" "#include \"${NAME_TITLE}Test\.moc\""

if ! tail -n 8 "${TEST_FILE}" | grep -q '\.moc"'; then
  echo "FAIL: .moc include is not near the end of the test file"
  FAILED=1
else
  echo "PASS: .moc include is near the end of the test file"
fi

if [[ "${FAILED}" -ne 0 ]]; then
  echo "==> Convention checks FAILED"
  exit 1
fi

echo "==> Building with tests to compile generated feature"
"${SCRIPT_DIR}/build.sh" --test ON

echo "==> Generator smoke test PASSED"
