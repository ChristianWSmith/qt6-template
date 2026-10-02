# Architectural & Design Audit — qt6-template

(Fresh pass; AUDIT-REPORT-2. Historical AUDIT-REPORT.md excluded by protocol.)

---

## Method and Constraints

Seven-agent audit (2A EventSystem, 2B ownership/persistence, 2C services/globals/logging/errors, 2D features/API/AI-ergonomics, 2E build/CI, 2F testing/performance, 2G un-workaround registry) plus verification. AUDIT-REPORT.md was never opened.

**Verification:** 121 assessed — **93 CONFIRMED**, **28 CONFIRMED-WITH-QUALIFICATION**, **0 NEEDS-EVIDENCE**, **0 REJECTED**. Severity after verification: Critical 0, Significant 5 agent-findings (**4 after dedupe**), Improvement 27, Opinion 11, Keep 56. Un-workaround classes: A=2, B=20 (UW-09 qualified), C=1 confirmed (UW-18; UW-13 → tooling), **D=0**, E=1 implicit (m_models **assert** — not a UW entry).

**Report presentation:** Section C Significant findings use full schema fields. Improvements and Opinions use compact table rows that still carry Location, Problem, Recommendation, and Evidence-bearing citations. Keep findings appear in the master register (AUD-K01–K48) with rationale in Section F — agent-level angle Keeps fold into canonical IDs with attribution preserved (agent counts 56 Keep vs register 48 canonical; Improvements 27 vs register 23 after dedupe/merge). This is consolidation, not omission.

**Rules:** Code is ground truth; AGENTS.md claims verified against implementation (Appendix A). Fresh-eyes rule; agent IDs re-namespaced `AUD-NNN` (2D-F-210 vs 2F-F-210 collision resolved). Evidence required; Windows path labeled inferred. No repo rewrites. Keep-dominant profile (56/99 agent findings) is correct — not inflation.

**Conflicts resolved by code evidence:**

| Conflict | Resolution |
|---|---|
| 2G UW-09 claims assert "catches forgotten appends"; 2D/2B show it does not for new features | **Code wins.** `AppMainWindow.cpp:57-62` hardcodes `findChildren<CounterModel*>() + findChildren<AppLogModel*>()`. UW-09 sentence not carried. List=Keep/B; assert=Significant/E. |
| 2E classifies 7-Zip aqt workaround as D; 2G as A | **2G wins.** py7zr bug still affects pinned aqt (`Pipfile:13`); class A. |
| 2E glob+fat-lib+greps=C; 2G glob alone=B | Complementary scopes: C in fat-lib context (AUD-107); B for glob facility. |
| 5 Significant agent-findings vs 4 report findings | 2B-F-2B-007 and 2D-F-210 are the **same** issue. Deduped to AUD-002 with dual attribution. |

---

## A. Executive Summary

### Overall assessment

A **technically sound, deliberately opinionated Qt6 template** whose documentation unusually matches its implementation. Feature-first MV* composition, explicit persistence seam, typed EventSystem for genuinely decoupled events, single-threaded GUI-thread persistence policy — each mechanism justified by a documented contract and locked by tests where load-bearing.

**No Critical findings.** Keep-dominant profile is not deflation; custom machinery mostly earns its rent. Four Significant issues: convention-only provider lifetime on the default path; broken extension-time m_models assert; Windows smoke path vs Ninja layout (inferred); EventSystem quit guards untested (code itself correct).

AI ergonomics **partly legible**: EventSystem/persistence/ownership strong; composition-root extension is the weak joint (manual checklist, broken census assert, "Model"/"Service" vocabulary collisions).

### Strongest decisions

1. **EventSystem as typed pub/sub scaffolding** — free-fn `[[nodiscard]] Subscription` RAII over `QCoreApplication` context, always-queued, all-builds GUI-thread policy, non-owning map cleared on `aboutToQuit`. Zero production publishers by design (Demo* naming). Class B.
2. **Persistence three-layer model** — `PersistenceResult<T>`/`PersistenceError` taxonomy (NotFound/IoError/InvalidData/CommitError), provider-owned diagnostics, log-and-continue. C++20 `std::expected` analogue.
3. **Dual-ctor persistence seam** — default `FilePersistenceProvider(this)` vs injected borrow, not reparented. Ownership mode is type-level.
4. **Service registration lifecycle** — explicit register/unregister on every main() path + qScopeGuard; static Subscription is storage not lifecycle. No macros.
5. **app.env SSOT fail-fast** — shell→Conan→CMake→PUBLIC compile defs; FATAL_ERROR/ConanInvalidConfiguration on missing fields.
6. **Test-mode isolation** — setTestModeEnabled + QSettings redirect before any fixture.
7. **Honest self-documentation** — EventSystem header contract; boundary script self-documents limitations; AGENTS.md documents QSettings gap accurately.

### Largest risks

1. **Provider sibling lifetime, convention-only on default path** (AUD-001) — provider+models are sibling Qt children; no sibling destruction-order guarantee. Safe today (no model dtors touch provider). Future dtor doing provider I/O = UB.
2. **m_models debug assert broken for extension** (AUD-002) — hardcodes two reference types. Forgotten append: assert passes, persistence silently lost. Correct append: assert false-positives. Trains deletion.
3. **Windows smoke path vs Ninja** (AUD-003, Medium, inferred) — build.sh expects `${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe`; profiles/windows pins Ninja → exe at `${BUILD_DIR}/${APP_NAME}.exe`. Smoke likely skips; PR CI on Windows never launches production binary.
4. **EventSystem quit guards untested** (AUD-004) — aboutToQuit/quitFired_ correct in code, locked by no test. **Test gap in lifetime-critical path, not a defect.** Related latent wart: quitHookInstalled_ never resets (AUD-123).

### Simplification opportunities

Fix/remove m_models assert (AUD-002); compose constructor wiring once (AUD-104); unify platform binary-path resolution (AUD-109); document vocabulary/channel splits in headers (AUD-117/118/201); close highest-value test gaps (AUD-004/101/102/114).

### Un-workarounds

**Zero historical (D) mechanisms.** A=2 (QVariant/moc — current limitation; py7zr→7-Zip — still required), B=20, C=1 (common.h, low impact), D=0, plus one E-class **guard defect** (m_models assert — not a custom mechanism). The custom machinery mostly **is** the product — pedagogical scaffolding for contracts Qt does not package. Do not trust the extension-time assert (E); do not call QVariant transport removable (A).

### Remain opinionated

EventSystem reserved for decoupled events (intra-feature = Qt signals); sync GUI-thread persistence + Async Policy; explicit services, no DI/macros; logging never routes through EventSystem; persistence taxonomy not collapsed; String boundary policy; custom.qss as stylesheet extension point; cxxopts/fmt as Conan demos.

### Thin coverage (honest)

- **C++20 idiom audit (§7) thin** — no dedicated ranges/spans/consteval sweep; no deep C++20-vs-C++23 justification beyond PersistenceResult.
- **Windows path inferred, not observed** — verify on Windows before treating as operational fact.
- **macOS packaging partial** — path duplication noted; no signing/notarization audit.
- **Performance LOW is structural reasoning** — no measurement; matches §17's own "optimize only with credible evidence."
- **AI-ergonomics deeper angles open** — provider destruction mid-session; logging-category discoverability from headers alone.

---

## B. Architecture Map

Condensed from ARCH-MODEL.md (reconstructed from code).

### Major subsystems

| Module | Role | Core files |
|---|---|---|
| `src/main.cpp` | CLI, logging install, QApplication, service lifecycle, translator, theme, smoke-test, exception paths | `main.cpp` |
| `src/appmainwindow/` | Composition root: dual ctors, feature triples, code-built layout, QSettings chrome, closeEvent save | `AppMainWindow.{h,cpp,ui}` |
| `src/core/` | `IModel`, `IPersistenceProvider` + `PersistenceResult`/`PersistenceError` | `IModel.h`, `IPersistenceProvider.h` |
| `src/events/` | Typed pub/sub: BusRegistry, EventDispatcher&lt;T&gt; via QVariant, Subscription RAII, GUI-thread policy | `system/EventSystem.hpp` |
| `src/features/{counter,applog}/` | Reference MV* features; keys `APP_ID ".CounterState"`/`".AppLogState"`; AppLog demonstrates EventSystem subscription | `model/`, `presenter/`, `widget/` |
| `src/logging/` | Five QLoggingCategories, custom messageHandler, configureLogLevel | `logging.{h,cpp}` |
| `src/platform/core/` | FilePersistenceProvider: JSON at AppDataLocation, QSaveFile, all persistence diagnostics | `FilePersistenceProvider.{h,cpp}` |
| `src/platform/theme/` | setTheme(): Windows Fusion + dark/light QSS; custom.qss concatenated last | `theme.hpp` |
| `src/services/` | Free-function EventSystem service + ServiceRegistry registerAll/unregisterAll | `ServiceRegistry.{hpp,cpp}` |
| `tests/` | Single UnitTests binary; test-mode isolation; suites for events/features/lifecycle/persistence/services/logging/themes | `tests/main.cpp` |

### Layering (no include cycles observed)

```text
                    +---------------------------+
                    |         main.cpp          |  bootstrap only
                    +-------------+-------------+
                                  |
                    +-------------v-------------+
                    |       AppMainWindow       |  composition root (fan-in)
                    +--+-------+-------+-------+
                       |       |       |
          +------------v-+ +---v----+ +v----------------+
          |  features/*  | | services| | platform/theme  |
          | model|pres|ui| | registry| | platform/core   |
          +--+----+---+--+ +---+----+ +--+------+--------+
             |    |   |        |         |      |
             v    v   v        v         v      v
          +--------------------------------------------+
          |  core/ (IModel, IPersistenceProvider)       |
          +--------------------------------------------+
                       ^ (allowed)
          +------------+------------+
          |   events/ (EventSystem) |-----> logging/
          +-------------------------+
```

Boundary rules 1–6 via `scripts/check-architecture-boundaries.sh` (grep/find heuristics; self-documented limitations lines 17–19) in `build.sh:52`, CTest (UNIX), CI Linux. Allowed edges: events→logging, services→events+logging, platform→core+logging, model→core+logging. Exemptions: presenters, appmainwindow, main.cpp, tests.

### Ownership (summary)

| Object | Owner | Encoding |
|---|---|---|
| AppMainWindow | main() stack | No WA_DeleteOnClose (LifecycleTest:50-53) |
| FilePersistenceProvider (default) | Qt child of window | Sibling of models — **no sibling destruction-order guarantee** (AUD-001) |
| Injected provider | Caller | Borrowed; not reparented; must outlive window |
| Feature model/widget/presenter | Qt children of window | `new X(this)`; layout reparents widgets |
| m_models entries | Non-owning `QList<IModel*>` | closeEvent loop only; IModel not a QObject |
| Event dispatchers | QCoreApplication (parented) | Registry never deletes; map cleared on aboutToQuit |
| Subscription (free-fn) | Caller must hold | RAII QMetaObject::Connection only; [[nodiscard]] |
| Service Subscription static | Storage only | Explicit unregisterAll is lifecycle; qScopeGuard backup |

### Event / data flow

**Counter (production — Qt signals):** Widget click → emit incrementRequested → Presenter (QPointer guard) → Model::increment → emit valueChanged → Presenter → Widget::displayCounter. Initial sync in presenter ctor (`CounterPresenter.cpp:18`) — model loads state in ctor *before* presenter exists.

**AppLog EventSystem demo (no production publisher):** `events::publish(DemoLogEvent{...})` → queued delivery → AppLogPresenter / DemoConsoleLogService. Grep src/: no production `events::publish` sites.

**Persistence (closeEvent):** hide() → QSettings chrome → `for (IModel* model : m_models) model->saveState()` → on error qCDebug + continue. Model ctors call loadState() at startup (NotFound=first-run; operational errors keep defaults; provider logged).

### Threading

Single GUI thread. No QThread/QtConcurrent in src/. EventSystem GUI-thread-only (all-builds qCCritical + debug Q_ASSERT); delivery always Qt::QueuedConnection. Bus mutex protects dispatcher-map creation only. Persistence synchronous by design (Async Policy non-goal, 7-step escalation).

### Init / shutdown

```text
main()
  cxxopts → handler → configureLogLevel → QApplication
  → services::registerAll() + qScopeGuard
  → AppMainWindow (model ctors loadState sync I/O) → setTheme
  → [optional --smoke-test → unregister + return 0]
  → exec()
  → closeEvent: hide + QSettings + m_models saveState loop
  → unregisterAll + removeTranslator → ~QApplication
```

Exception paths (std::exception and catch(...)) both call unregisterAll + return 1 (`main.cpp:110-119`).

---

## C. Findings

IDs `AUD-NNN`; agent attribution in parentheses. Significant in full schema; Improvements compact; Opinions table. Keep findings in §F + master register.

### Significant findings

---

#### AUD-001 — Provider sibling lifetime convention-only on default path

| Field | Value |
|---|---|
| **ID** | AUD-001 |
| **Severity** | Significant |
| **Confidence** | High |
| **Location** | `AppMainWindow.cpp:12-27`; `IPersistenceProvider.h:114-118`; `IModel.h:27-30`; `AppMainWindow.h:40-41,56-58` |
| **Category** | Ownership / Lifetime |
| **UnWorkaroundClass** | N/A (convention gap) |

**Current Design:** Default path: provider + models all QObject children of AppMainWindow; provider first in init-list (`:17` before models `:18-25`). Contract = construction order + doc that model dtors must not dereference provider + fact that I/O happens only in loadState (ctor) and saveState (closeEvent). No model declares a destructor. Injected path (`:29-43`) borrows caller-owned provider outside the window tree — structural.

**Problem:** Provider-outlives-models is **convention-only on the default path**. Provider is a *sibling* child, not an ancestor. Qt guarantees parent deletes children but not sibling order; repo states this itself (`AppMainWindow.h:40-41`). Future model dtor doing provider I/O → reference may dangle — UB, not a debug assert. Production is safe **today** because no model touches provider in its dtor. Hazard is for **future extension**.

**Why It Matters:** AGENTS.md flags dtor rule as silent-failure #6; enforcement is documentation + "current models happen to have no dtors." Injected path structurally safer (lifecycle test authors rely on caller declaration order, `LifecycleTest.cpp:142-143`). Template consumers will add features; default path has no mechanical protection.

**Canonical Mechanism:** No Qt facility makes a sibling QObject outlive another. Value/unique_ptr **member** provider is strictly worse — C++ destroys members *before* base-class dtor; `~QObject` (deletes child models) runs in base chain last.

**Alternatives:**
- **A — Keep convention:** simplest; documented; teardown tests never exercise provider I/O in dtors.
- **B — Structural via main() injection (recommended if contract should be structural):** stack-allocate `FilePersistenceProvider provider;` in main() *before* `AppMainWindow mainWindow(provider);` use injected ctor in production. Stack destruction order guaranteed: provider declared first destroyed last. ~2 lines. Default ctor remains simple-app/test path.
- **C — Reparent models under provider:** dtors run while provider mid-destruction — virtual calls from model dtor still UB. **Not recommended.**
- **D — Eliminate stored provider reference:** pass provider into load/save calls. Removes dangling-ref class; weakens pedagogy; breaks generated-model convention.
- **E — Value-member provider:** rejected — strictly worse than siblings.

**Recommendation:** Do not rewrite dual-ctor seam. Prefer **B** for structural contract in production; else **A** with (1) existing docs, (2) AGENTS.md + generate.sh comments stating dtor rule is convention-only on default path, (3) never adding model dtors that touch provider. Keep closeEvent as only shutdown-persistence site.

**Migration Complexity:** Low (B: ~2 lines; A: docs only) | **Risk:** Low
**Evidence:** `AppMainWindow.cpp:17-25`; `AppMainWindow.h:40-41,56-58`; `IPersistenceProvider.h:114-118`; `CounterModel.h`/`AppLogModel.h` (no dtors); `LifecycleTest.cpp:142-143`; `main.cpp:89`; Qt QObject docs; C++ [class.dtor]. *(2B-F-2B-006)*

---

#### AUD-002 — m_models debug assert broken for extension

| Field | Value |
|---|---|
| **ID** | AUD-002 |
| **Severity** | Significant |
| **Confidence** | High |
| **Location** | `AppMainWindow.cpp:53-62`; AGENTS.md E10/H9; `generate.sh:390-397`; `LifecycleTest.cpp` (stock counts hardcoded) |
| **Category** | Composition / Testing / Claim Verification |
| **UnWorkaroundClass** | **E for assert** (actively misleading); **B for m_models list** (Keep) |

**Current Design:**

```cpp
#ifndef NDEBUG
  const int modelChildren =
      findChildren<CounterModel *>().size() +
      findChildren<AppLogModel *>().size();
  Q_ASSERT(modelChildren == m_models.size());
#endif
```

Comment claims this catches forgotten appends. AGENTS.md: "assert the list size matches constructed feature models." generate.sh checklist omits census update. The **list** (`AppMainWindow.h:102-106`) is non-owning `QList<IModel*>` driving closeEvent — justified because IModel is not a QObject (`findChildren<IModel*>` impossible; code admits at `:54-55`).

**Problem:** Two defect modes for the template's primary extension path:
1. **False negative (forgotten append):** census stays 2, list stays 2 — assert passes while FooModel silently never saves. Exactly the silent failure the assert targets.
2. **False positive (correct extension):** append FooModel → list=3, census=2 → Q_ASSERT fires on **correct** code unless census is hand-edited (undocumented).

NDEBUG compiles assert out entirely. AGENTS.md E10/H9 overstates what code does. Assert punishes correct extension → trains deletion. Agents following checklist hit mode 2 on first added feature. LifecycleTest hardcodes stock counts; test-generator never wires AppMainWindow.

**Why It Matters:** m_models append is silent-failure #2. This assert is the only mechanical guard and does not guard the extension case. **Split:** list = Keep/B; assert as extension guard = Significant/E.

**Canonical Mechanism:** Generic collection impossible (IModel not QObject). Options: remove assert + rely on lifecycle e2e + generated wiring test; registration helper single-sourcing appends; generated census; or (weakest) document census must be updated manually.

**Alternatives:**
- **A — Remove Q_ASSERT:** honest reduction of false confidence; keep LifecycleTest e2e as real lock.
- **B — Registration helper (recommended):** private `registerModel(IModel*)` — single append site; list size = count of registered models by construction.
- **C — Generated census + checklist:** emit findChildren lines via generate.sh; checklist step "update census."
- **D — Keep assert + fix AGENTS.md wording only:** mode 1 remains. Weakest.

**Recommendation:** Prefer **B** or **A**. Update AGENTS.md E10/H9 to match guard that remains. **Do not carry UW-09's "assert catches forgotten appends" sentence.**

**Migration Complexity:** Low (A/B); Medium (generated wiring test) | **Risk:** Low
**Evidence:** `AppMainWindow.cpp:51-62`; AGENTS.md E10/H9; `generate.sh:390-397`; LifecycleTest stock-count hardcoding; 2D-F-210 / 2B-F-2B-007 (verified); UW-09 sentence rejected.

---

#### AUD-003 — Windows smoke-test path vs Ninja single-config layout

| Field | Value |
|---|---|
| **ID** | AUD-003 |
| **Severity** | Significant |
| **Confidence** | **Medium — static analysis, not observed on Windows** |
| **Location** | `build.sh:77-83`; `run.sh:52-59`; `configure-vscode.sh:14-20`; `conan/profiles/windows:11`; `conanfile.py:53-61`; `windows-bundle.sh:15-24` |
| **Category** | Build / CI |
| **UnWorkaroundClass** | D (VS-era layout assumption vs pinned Ninja) |

**Current Design:** Windows smoke path `${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe` (multi-config). `run.sh:52-54` same pattern. `configure-vscode.sh:15` hardcodes `${BUILD_DIR}/Debug/...`. `conanfile.py:57-58` DLL copy to `${build_folder}/${build_type}`. **Meanwhile** `conan/profiles/windows:11` pins `generator=Ninja` (single-config → exe at `${BUILD_DIR}/${APP_NAME}.exe`). `windows-bundle.sh:15-24` has explicit multi/single-config fallback — authors know both layouts occur.

**Problem:** Three scripts + conanfile encode multi-config assumption contradicting pinned Ninja. Under Ninja, build.sh:84-88 won't find Release/MyApp.exe → WARNING + **skips** smoke without failing. run.sh rebuild guard sees missing path every invocation. AGENTS.md "platform-aware path like run.sh" is form-true but Windows form likely misses actual output.

**Why It Matters:** PR CI on Windows never launches production binary (bundle re-smoke only on main pushes, which has fallback). Dev-loop smoke is silent no-op under the most common Windows config this repo pins.

**Canonical Mechanism:** CMake single-config Ninja (already pinned); shared script helper trying single-config first; CMake File API for target paths.

**Qualification:** **Inferred, not observed.** Medium confidence honest. Label: "highly likely silent smoke-test skip on Windows dev-loop; verify on Windows before treating as confirmed."

**Alternatives:** **A (recommended)** — align on Ninja via shared helper; missing smoke target = hard failure in CI. B — pin VS generator (regresses speed). C — CMake helper script (most robust, more glue).

**Recommendation:** **A** via shared path helper (AUD-109). Verify on Windows. Fix configure-vscode.sh path; consider conanfile.py DLL-copy directory.

**Migration Complexity:** Medium | **Risk:** Low
**Evidence:** `build.sh:77-88`; `run.sh:52-59`; `configure-vscode.sh:14-20`; `conan/profiles/windows:11`; `conanfile.py:53-61`; `windows-bundle.sh:15-24`; `ci.yml:80-82`; 2E-F-2E-006 (verified, Medium).

---

#### AUD-004 — EventSystem quit guards untested (test gap in lifetime-critical path)

| Field | Value |
|---|---|
| **ID** | AUD-004 |
| **Severity** | Significant **as test gap** — NOT "EventSystem broken" |
| **Confidence** | High |
| **Location** | `EventSystem.hpp:341-349` (aboutToQuit clear), `:236-240`, `:266-270`, `:307-311` (quitFired_ guards), `:356-362`; `EventsTest.cpp` (zero aboutToQuit/quitFired_ refs) |
| **Category** | Testing / EventSystem / Lifetime |
| **UnWorkaroundClass** | N/A (test gap) |

**Current Design:** BusRegistry installs aboutToQuit on first dispatcher creation; sets quitFired_; clears non-owning map while dispatchers still exist. Post-quit publish/subscribe refuse via qCCritical + no-op. Registry never deletes dispatchers; parented to QCoreApplication. **Code itself is correct** — verified by 2A-F-006, claims A12/A13, direct code read.

**Problem:** Teardown path untested. EventsTest covers delivery, Subscription lifetime, white-box parenting, FIFO, type-mismatch, in-flight-after-reset — never fires aboutToQuit, never asserts post-quit refusal is safe, never asserts map-clear timing doesn't dereference destroyed dispatchers. Most lifetime-sensitive code in EventSystem (raw pointers vs QObject ownership at shutdown).

**Why It Matters:** Regression in quitFired_ ordering or map-clear timing → UAF or recreate-dispatcher-against-dying-app at shutdown. Contract documented but convention-only in tests. **Frame as test gap, not defect.** Cross-ref AUD-123 (latent multi-app dead bus — different problem, same code region; unreachable today — single QApplication in main.cpp:62 and tests/main.cpp:36).

**Canonical Mechanism:** aboutToQuit observable via fixture observer, nested app harness, or subprocess tests. Minimum: publish after instance()==nullptr logs critical, does not deliver.

**Alternatives:** **A** — in-process shutdown-ordering test. **B** — subprocess death-test. **C** — document-only (current; unacceptable given stakes vs test cost).

**Recommendation:** Add test locking: (1) after quit fired/app gone, publish/subscribe don't deliver, don't crash; (2) map entries not dereferenced after clear. Prefer A if feasible; B otherwise. Highest-value test gap in suite.

**Migration Complexity:** Medium (test-only) | **Risk:** Low
**Evidence:** `EventSystem.hpp:341-349`, `:236-240`/`:266-270`/`:307-311`, `:356-362`; EventsTest grep → none; 2F-F-207 (verified as test gap); 2A-F-006 (code correct); 2A-F-009 (latent, separate).

---

### Improvements

Compact format. All schema fields present; tighter prose.

| ID | Title | Severity/Conf/Class | Location | Problem (compressed) | Recommendation | Agent(s) |
|---|---|---|---|---|---|---|
| AUD-101 | Wrong-thread rejection path untested | Imp / High / N/A | EventsTest (no QThread); EventSystem.hpp:154-165,221-232,250-262,292-303 | Policy enforced in code at 4 sites; zero behavioral test. Runtime guard regression ships silently in release. Wrong-thread returns **before** Q_ASSERT — path testable in debug builds. | Add wrong-thread publish + free-fn subscribe tests (worker thread + qWait + handler probe). | 2A-F-011, 2F-F-208 |
| AUD-102 | LoggingTest shallow | Imp / High / N/A | LoggingTest.cpp:9-28; logging.cpp:107-134 | Three SUCCEED()-only tests; filter-rule application unasserted. Token→rule mapping regressions pass. | Handler-capture behavioral tests: configureLogLevel("debug") → probe qCDebug captured; "error" → not captured; unknown token leaves rules intact. QLoggingCategory has no public rules-read API; capture is canonical black-box. | 2C-F-2C-010, 2F-F-206 |
| AUD-103 | QSettings chrome zero failure observation | Imp / High / N/A | AppMainWindow.cpp:64-66,90-92 | setValue/restore with no status()/sync()/logging. Asymmetric with feature-state channel. Doc describes gap accurately; doesn't close it. | After setValue loop, check settings.status(); log warning on non-NoError; never block close. Do not merge channels. | 2C-F-2C-008 |
| AUD-104 | Composition wiring convention-only + ctor duplication | Imp / High-Med / N/A | AppMainWindow.cpp:12-43; generate.sh:390-397 | Both ctors duplicate full feature sequence. Four silent-failure points documented but non-binding. test-generator never wires AppMainWindow. | (A) private constructFeatures() from both ctors after provider bind — lowest risk. (B) attach(model, widget) helper binds m_models+layout in one call. (C) CI test wiring probe feature. Do not change dual-ctor provider semantics. | 2D-F-211, 2B-F-2B-009 |
| AUD-105 | QObject subscribe returns void (no handle) | Imp / High / C | EventSystem.hpp:246-282 vs :289-324 | QObject overload void; failures log-only. Free-fn returns Subscription. No precise early unsubscribe for QObject receivers. | Prefer returning Subscription from QObject overload (Option A); interim document log-only semantics (Option C). Do not add exceptions. | 2A-F-010, 2C-F-2C-009 |
| AUD-106 | Generator fidelity gaps | Imp / High / N/A | generate.sh vs CounterWidget.cpp:2,7 / AppLogWidget.cpp:2,8 | Generated widgets omit logging include + instantiation line reference widgets have. loadState commented — intentional. Stub findChild hardcodes "incrementButton" — harmless. | Add logging include + instantiation line to widget template; reword stub example. Keep loadState as commented scaffold. | 2D-F-214 |
| AUD-107 | Glob + fat-lib + grep boundaries | Imp / Medium / C (fat-lib) B (glob alone) | CMakeLists.txt:32-39,55-73; boundary script | Single fat _lib + PUBLIC src/ means CMake graph cannot express layering; only machine-enforced boundary layer is evadable grep. CONFIGURE_DEPENDS unreliable on some generators. | Keep globs + heuristic script at template scale; document generators where CONFIGURE_DEPENDS fallback applies. Multi-target CMake only if template evolves into larger app. Green script run ≠ proof. | 2E-F-2E-007; UW-23 |
| AUD-108 | Dockerfile/profile toolchain drift | Imp / Medium / C | Dockerfile:1,5-84; conan/profiles/linux; ci.yml:69-77 | ubuntu:22.04 gcc~11 vs profiles/linux gcc-15. Conan doesn't verify compiler matches profile. CI "Toolchain versions" echoes only. | Install gcc-15 in Dockerfile + CI guard failing when runner compiler major ≠ profile. Do not "fix" by adding system Qt (Dockerfile is toolchain-only by design). | 2E-F-2E-010 |
| AUD-109 | Platform path logic duplicated ×5 | Imp / High / C | build.sh, run.sh, configure-vscode.sh, windows-bundle.sh, mac-bundle-app.sh, conanfile.py:53-61 | Same concern implemented independently with different fallbacks. Drift already caused AUD-003. | Extract scripts/lib/resolve-app-path.sh; single-config first, multi-config fallback; fail loudly under CI. Fold AUD-003 fix in. | 2E-F-2E-009 |
| AUD-110 | Hardcoded AUTOUIC autogen include path | Imp / High / B | CMakeLists.txt:62-73 | INTERFACE include hardcodes ${APP_NAME}_lib_autogen/include. Generator-expression fix evaluated empty on same target — tried, rejected. Layout change breaks consumers. | Re-test generator expression in **consumer** context (same-target emptiness may not apply); if works, replace. Else keep — comment prevents incorrect fix. | 2E-F-2E-008 |
| AUD-111 | AppImage downloads unpinned | Imp / High / C-D | linux-bundle-appimage.sh:16-19,52-60 | appimagetool from "continuous" channel; excludelist from master raw URL. No pins, no checksums. Rest of pipeline unusually pinned. | Pin appimagetool release tag + sha256; vendor excludelist in-repo. | 2E-F-2E-014 |
| AUD-112 | pipenv --deploy parity gap | Imp / Medium / C | env.sh:4-12; build.sh:49; ci.yml:43-49,83-87 | installPipenv lacks --deploy; CI uses it. Dev runs may silently relock if Pipfile/lock diverge. UW-13 reclassified to tooling. | Use --deploy in installPipenv (or document --skip-lock choice). uv migration later as tooling modernization. | 2E-F-2E-017; UW-13 |
| AUD-113 | MemoryPersistenceProvider fidelity gap | Imp / Medium / N/A | MemoryPersistenceProvider.h:26-30; FilePersistenceProvider.cpp:40-56 | No doc.isObject() check — non-object JSON returns success(empty) vs File's InvalidData. Parse-error path dead through public API (storage always object JSON). Unreachable today (models treat non-NotFound identically); risk rises if future model branches on InvalidData from corrupted bytes while tests use Memory. | Add `if (!doc.isObject()) return failure(InvalidData);` for parity. Document Memory as API-boundary double, not storage-behavior clone. | 2F-F-213 |
| AUD-114 | main.cpp exception-path unregister untested | Imp / Medium / N/A | main.cpp:68-70,104-119 | Explicit unregisterAll + qScopeGuard correct by inspection. ServiceRegistrationTest covers API idempotence, not main() call sites. | Low-cost: subprocess/exit-code smoke variants, or extract main() logic for testability. Acceptable to defer — structural backup correct by inspection. | 2F-F-215 |
| AUD-115 | Thread-guard asymmetry | Imp / High / C | IModel.h:24; IPersistenceProvider.h:114-115 vs EventSystem.hpp guard sites | Models/providers document GUI-thread-only; EventSystem runtime-guards all builds. Different misuse-resistance standards for same error class. | Debug asserts in FilePersistenceProvider load/save + reference model mutators; generator emits same. Runtime guards only if worker threads enter scope per Async Policy. Document asymmetry in AGENTS.md. | 2D-F-215 |
| AUD-116 | AppLogWidget setLogMessages appends; trim re-encoded | Imp / High / C | AppLogWidget.cpp:13-37; AppLogWidget.h:27-29 | setLogMessages calls addItems **without clear** — name promises replacement, impl appends; twice = silent duplicates. handleLogChanged re-encodes model trim protocol (delete takeItem(0) if trimmed) — second implementation of model state. Correctness depends on accurate trimmed deltas forever. Tested, low-risk at kMaxLogSize=100. | Full-state refresh: setLogMessages clears then adds; presenter calls setLogMessages(model->getLogMessages()) on every change; delete handleLogChanged/LogDelta from widget API. Fallback: rename to appendLogMessages + document. | 2D-F-212 |
| AUD-117 | ServiceRegistry naming over-promise | Imp / Medium / N/A | ServiceRegistry.hpp; AGENTS.md Service Registration | "services" namespace wires **only** free-function EventSystem subscriptions. Header doesn't state "EventSystem-subscriptions-only" scope. | Header comment: "retains free-function EventSystem subscriptions only; Qt-object services use QObject parent-child; no general DI container." Do not expand mechanism scope. | 2D-F-217 |
| AUD-118 | Config-channel split discoverability | Imp / Medium / C | IPersistenceProvider.h:112-118; AppMainWindow.cpp:64-66,90-92; AppMetadata.h | QSettings (chrome) vs IPersistenceProvider (feature state) split documented in AGENTS.md table; weakly from headers. AppMetadata.h covers APP_* SSOT, not channel split. Key naming documented, intentionally unenforced (no key registry — non-goal). | Two short header comments: AppMainWindow.h (chrome) + IPersistenceProvider.h (feature-state only). Keep key-registry non-goal. Complementary to AUD-103. | 2D-F-219 |
| AUD-119 | Generator initial-sync check comment-only | Imp / High / B | test-generator.sh:102-103,123-140; generate.sh:316-323,370-372 | Greps comment string; stub is Placeholder{EXPECT_TRUE(true)}. Presenter could keep comment, omit sync call — all checks still pass. | Generate non-trivial stub: seed MemoryPersistenceProvider, construct MV triple, assert view shows restored state. Tighten presenter check to require non-comment sync call, or accept behavior locked by generated test. | 2F-F-209 |
| AUD-120 | Cross-subscriber order documented but untested | Imp / High / N/A | EventSystem.hpp:27-29; EventsTest.cpp:175-187; :593-610 | Contract documents cross-subscriber order as untested Qt property. MultipleSubscribersReceiveEvents asserts count only. | Add ordering test (subscribe A then B; publish; expect [A,B]). Closes last behavioral-contract test gap in EventsTest alongside AUD-004/101. | 2A-F-012 |
| AUD-121 | QSS list duplicated qrc vs tests | Imp / High / C | resources.qrc; tests/CMakeLists.txt:32-46 | Same five QSS files in production qrc and tests (tests don't link exe). No drift check between lists. | Derive tests QSS list from resources.qrc (file STRINGS parse), or qt_add_library from qrc linked by exe+tests. Low priority if styles list considered stable. | 2E-F-2E-012 |
| AUD-122 | inno.iss left at PROJECT_ROOT after packaging | Imp / Low / C | windows-bundle.sh:41-61; .gitignore:110 | Windows packaging writes generated `inno.iss` to `PROJECT_ROOT` and does not remove it. **.gitignore:110 already ignores `inno.iss`** — no accidental-commit risk. Residual issue is workspace hygiene only (generated artifact left in source tree; may confuse humans/agents scanning the root). | Emit `inno.iss` under DIST_DIR (or a temp dir) + cleanup trap on exit. Do **not** "add to .gitignore" (already present). Low priority. | 2E-F-2E-018 |
| AUD-123 | EventSystem quit flags never re-arm (latent multi-app) | Imp / Medium / C | EventSystem.hpp:341-352,375-379 | quitHookInstalled_ set once, never reset; quitFired_ set in hook, never cleared. Process destroying QCoreApplication and constructing new one → bus permanently dead (hook not reinstalled; publish/subscribe refuse). **Unreachable today** — single QApplication per process. Latent correctness wart in reusable facility; not "bus broken today." | (A) Re-arm hook per QCoreApplication instance; clear quitFired_ on new app. (B) Document "one QCoreApplication per process" + assert on recreation. Do B at minimum if A defers. Cross-ref AUD-004. | 2A-F-009 |

---

### Opinions (selected)

| ID | Title | Severity/Conf | Location | Problem | Recommendation | Agent |
|---|---|---|---|---|---|---|
| AUD-201 | "Model" vocabulary collision | Opinion/High | IModel.h; features/*/model/; no QAbstract* in src/ | Template "Model" = application state (IModel: QObject+persist+signals). No Qt Model/View classes in src/. AGENTS.md never discusses QAbstractListModel when appropriate. Vocabulary collision costs agents a reasoning step; can produce wrong-scaffold code. | Document in AGENTS.md + IModel.h: "This Model is application state, not QAbstractItemModel. For item views prefer QAbstractListModel." Avoid rename IModel→IFeatureState (breaking rename has own pedagogical cost). | 2D-F-213 |
| AUD-202 | loadState void absorbs corruption | Opinion/Med | IModel.h:17-21,41-42; CounterModel.cpp:27-35; AppLogModel.cpp:43-51 | loadState() void; NotFound=first-run; operational errors absorbed (provider logged). UI layer: corrupt vs never-saved indistinguishable. App needing "data was reset because corrupt" has no hook through IModel. Intentional, documented, consistently applied — legitimate tradeoff. | Keep void contract. AGENTS.md consumer-facing section: apps needing load-error UX must extend contract deliberately — don't let agents "fix" with ad-hoc load logging or parallel APIs. Future contract change: return PersistenceResult from loadState or optional stateLoadFailed signal. | 2C-F-2C-007 |
| AUD-203 | QPointer pedagogy (redundant for stock graph) | Opinion/High | CounterPresenter.h:21-27; AppLogPresenter.h:28-34; CounterTest.cpp:138-188 | QPointer+guards+no-dtor-deref. Within sample app's own graph (all Qt children; closeEvent before destruction; auto-disconnect), raw non-owning pointers sufficient — QPointer could look like ceremony. Residual: ctor initial sync not null-guarded (CounterPresenter.cpp:18). Both agents' framings correct at different levels: redundant for stock graph; valuable as template pedagogy for consumers who destroy feature objects independently. | Keep as documented presenter convention. Necessity is pedagogical, not structural. Optionally guard/document ctor initial-sync null case (minor). Do not remove because stock graph makes it unnecessary. | 2B-F-2B-008, 2D-F-209 |
| AUD-204 | ReusableWidget teaching artifact | Opinion/High | ReusableWidget.{h,cpp,ui} | Compiled into _lib via GLOB; never instantiated by sample app. Value: matches generate.sh widget shape; shows standalone-widget conventions. Confusion risk low-moderate for agents scanning src/widgets/ — mitigated by header comment + AGENTS.md teaching-artifact status. | Keep for now. If cleanup prunes artifacts, move to examples/ before deleting. Ensure agent prompts treat as non-production. | 2D-F-218 |
| AUD-205 | common.h ceremony | Opinion/High / C | countercommon.h (empty); applogcommon.h (LogDelta); generate.sh:167-182 | Always emitted. Rule 1 forbids widgets including model headers — common.h is correct place for widget-visible shared types. Empty stub = indirection for simple features. | Keep always-emit for uniform discoverability, or optionally skip empty files. Not worth dedicated migration. UW-18 C confirmed. | 2D-F-216, UW-18 |
| AUD-206 | Timestamp-in-string persistence | Opinion/High | AppLogModel | Timestamps prefixed into persisted strings; demo disclaimer in header. Production-shape weakness real (structured data flattened) but documented pedagogy — not a recommendation to persist production diagnostic logs. | Keep as documented demo. Production apps store structured timestamps separately if needed. | 2D-F-222 |
| AUD-207 | registerAll void silent no-op | Opinion/Med | ServiceRegistry.{hpp,cpp}; EventSystem.hpp:292-311 | registerAll returns void; Subscriptions may be empty if EventSystem guards reject. Stock main() enforces ordering (QApplication :62 before registerAll :68) — failure path unreachable. Header documents happy-path only. Not Critical: guards log qCCritical; unregister on empty Subscription safe. | (A) Extend ServiceRegistry.hpp lifecycle comment with precondition. (C) If touching registerAll, debug-only Q_ASSERT(isConnected()) after subscribe — matches EventSystem's own idiom. No custom exception framework. | 2C-F-2C-011 |
| AUD-208 | free-fn fn-ptr only (lambda prohibition) | Opinion/High / B | EventSystem.hpp:289-290 | subscribe(void (*)(const T&)) only. No lambda/functor/std::function overload. Prevents capturing lambdas (reintroducing lifetime hazards Subscription exists to avoid for stateless handlers). Real ergonomic cost for future stateful services: must be QObjects or use global/static state. | Keep fn-ptr-only; state rationale in free-fn subscribe docs (captures reintroduce hazards; use QObject receivers for stateful handlers). Evaluate functor overload only if concrete stateful free-fn service appears. | 2A-F-013 |
| AUD-209 | static BusRegistry singleton | Opinion/Med / B | EventSystem.hpp:368-371; ServiceRegistry.cpp:13 | Process-global by design. Free-function bus must be reachable without injection; injected bus interface makes free-fn facade awkward (facade needs global pointer anyway). Storage-vs-lifecycle distinction documented and real. Test-order sensitivity: one QApplication + one BusRegistry for whole UnitTests binary — leak risk if not cleaned up (works via scoping/unregisterAll). | Keep static singleton. Document that EventSystem test isolation relies on subscriber scoping/unregisterAll, not fresh bus per test. | 2A-F-015 |
| AUD-210 | conan.lock superset | Opinion/High / B | conan.lock; conan-lock-update.sh | Lock records xorg/system, cmake, meson, pkgconf beyond recipe declarations. Looks like corruption; "cleaning" lock to match recipe is obvious temptation. Conan lockfiles legitimately record resolved graph including profile-driven system-package integrations. Merge script uses atomic-replace safety. | Keep. Never hand-prune conan.lock; regenerate via conan-lock-update.sh. | 2E-F-2E-016 |
| AUD-211 | Directory-scope CMake globals | Opinion/High / C | CMakeLists.txt:14,16-17,30 | EXPORT_COMPILE_COMMANDS, CXX_STANDARD 20, AUTORCC ON as directory-scope globals. Conventional Qt6 style; inheritance desired (tests compile as C++20, process AUTOMOC for Q_OBJECT receivers). No observed harm. | Keep. Revisit only if a target needs divergent standard/AUTORCC settings. | 2E-F-2E-020 |

---

### Master findings register (all findings including Keep)

| ID | Title | Severity | Conf. | Category | Agent(s) |
|---|---|---|---|---|---|
| **AUD-001** | Provider sibling lifetime convention-only (default path) | Significant | High | Ownership | 2B-F-2B-006 |
| **AUD-002** | m_models debug assert broken for extension | Significant | High | Composition/Testing | 2D-F-210 (canon), 2B-F-2B-007 |
| **AUD-003** | Windows smoke path vs Ninja layout | Significant | Medium | Build | 2E-F-2E-006 (inferred) |
| **AUD-004** | EventSystem quit guards untested | Significant (test gap) | High | Testing/EventSystem | 2F-F-207 |
| AUD-101 | Wrong-thread rejection path untested | Improvement | High | Testing/Concurrency | 2A-F-011, 2F-F-208 |
| AUD-102 | LoggingTest shallow | Improvement | High | Testing/Logging | 2C-F-2C-010, 2F-F-206 |
| AUD-103 | QSettings observation gap | Improvement | High | ErrorHandling | 2C-F-2C-008 |
| AUD-104 | Composition wiring + ctor duplication | Improvement | High/Med | Composition | 2D-F-211, 2B-F-2B-009 |
| AUD-105 | QObject subscribe no handle | Improvement | High | EventSystem/API | 2A-F-010, 2C-F-2C-009 |
| AUD-106 | Generator fidelity gaps | Improvement | High | CodeGen | 2D-F-214 |
| AUD-107 | Glob + fat-lib + grep boundaries | Improvement | Medium | Build | 2E-F-2E-007 |
| AUD-108 | Dockerfile/profile drift | Improvement | Medium | Build/CI | 2E-F-2E-010 |
| AUD-109 | Platform path logic duplicated | Improvement | High | Build | 2E-F-2E-009 |
| AUD-110 | Hardcoded autogen include path | Improvement | High | Build | 2E-F-2E-008 |
| AUD-111 | AppImage downloads unpinned | Improvement | High | Build/CI | 2E-F-2E-014 |
| AUD-112 | pipenv --deploy parity gap | Improvement | Medium | Build | 2E-F-2E-017 |
| AUD-113 | MemoryPersistenceProvider fidelity | Improvement | Medium | Testing/Persistence | 2F-F-213 |
| AUD-114 | Exception-path unregister untested | Improvement | Medium | Testing/Services | 2F-F-215 |
| AUD-115 | Thread-guard asymmetry | Improvement | High | Concurrency/API | 2D-F-215 |
| AUD-116 | AppLogWidget setLogMessages appends | Improvement | High | Feature/API | 2D-F-212 |
| AUD-117 | ServiceRegistry naming over-promise | Improvement | Medium | API/Discoverability | 2D-F-217 |
| AUD-118 | Config-channel discoverability | Improvement | Medium | API/Persistence | 2D-F-219 |
| AUD-119 | Generator initial-sync comment-only | Improvement | High | Testing/AI-Erg | 2F-F-209 |
| AUD-120 | Cross-subscriber order untested | Improvement | High | Testing/EventSystem | 2A-F-012 |
| AUD-121 | QSS list duplicated | Improvement | High | Build | 2E-F-2E-012 |
| AUD-122 | inno.iss left at PROJECT_ROOT (workspace hygiene) | Improvement | Low | Build | 2E-F-2E-018 |
| AUD-123 | EventSystem quit flags never re-arm (latent) | Improvement | Medium | EventSystem/Lifecycle | 2A-F-009 |
| AUD-201 | "Model" vocabulary collision | Opinion | High | API/AI-Erg | 2D-F-213 |
| AUD-202 | loadState void corruption absorption | Opinion | Medium | ErrorHandling | 2C-F-2C-007 |
| AUD-203 | QPointer pedagogy | Opinion | High | Ownership | 2B-F-2B-008, 2D-F-209 |
| AUD-204 | ReusableWidget teaching artifact | Opinion | High | Feature | 2D-F-218 |
| AUD-205 | common.h ceremony (class C) | Opinion | High | Complexity | 2D-F-216, UW-18 |
| AUD-206 | Timestamp-in-string persistence | Opinion | High | Feature | 2D-F-222 |
| AUD-207 | registerAll void silent no-op | Opinion | Medium | Services | 2C-F-2C-011 |
| AUD-208 | free-fn fn-ptr only | Opinion | High | EventSystem/API | 2A-F-013 |
| AUD-209 | static BusRegistry singleton | Opinion | Medium | GlobalState | 2A-F-015 |
| AUD-210 | conan.lock superset | Opinion | High | Build | 2E-F-2E-016 |
| AUD-211 | directory-scope CMake globals | Opinion | High | Build | 2E-F-2E-020 |
| AUD-K01 | EventSystem typed pub/sub scaffolding | **Keep** | High | EventSystem | 2A-F-001, UW-01 |
| AUD-K02 | QVariant transport (moc constraint) | **Keep** | High | EventSystem | 2A-F-002, UW-02 (class A) |
| AUD-K03 | Queued delivery + GUI-thread policy | **Keep** | High | Concurrency | 2A-F-003, UW-24 |
| AUD-K04 | Subscription RAII + [[nodiscard]] | **Keep** | High | Ownership | 2A-F-004, UW-21 |
| AUD-K05 | Free-fn subscribe + QCoreApplication context | **Keep** | High | Ownership | 2A-F-005, UW-05/19 |
| AUD-K06 | BusRegistry lifetime design | **Keep** | High | GlobalState | 2A-F-006, 2C-F-2C-002 |
| AUD-K07 | Logging/EventSystem separation | **Keep** | High | Dependency | 2A-F-007 |
| AUD-K08 | Demo-only DemoLogEvent (pedagogy) | **Keep** | High | EventSystem | 2A-F-008 |
| AUD-K09 | No filter/transform/error channel (Qt parity) | **Keep** | High | EventSystem | 2A-F-014 |
| AUD-K10 | Dual-ctor persistence seam | **Keep** | High | Ownership | 2B-F-2B-001, UW-08 |
| AUD-K11 | PersistenceResult + taxonomy + provider logging | **Keep** | High | Persistence | 2B-F-2B-002, UW-03/04 |
| AUD-K12 | closeEvent log-and-continue + stack window | **Keep** | High | Lifecycle | 2B-F-2B-003 |
| AUD-K13 | FilePersistenceProvider QSaveFile + APP_ID keys | **Keep** | High | Persistence | 2B-F-2B-004 |
| AUD-K14 | m_models list (NOT the assert) | **Keep** | High | Ownership | 2B-F-2B-005, UW-09 (list=B) |
| AUD-K15 | Mechanism census (no custom lifetime framework) | **Keep** | High | Ownership | 2B-F-2B-010 |
| AUD-K16 | Service lifecycle explicit + qScopeGuard | **Keep** | High | Services | 2B-F-2B-011, 2C-F-2C-001, UW-25 |
| AUD-K17 | Theme/translator lifetime | **Keep** | High | Ownership | 2B-F-2B-012 |
| AUD-K18 | Explicit service registration (no macros) | **Keep** | High | Services | 2C-F-2C-004 |
| AUD-K19 | Logging categories + configureLogLevel | **Keep** | High | Logging | 2C-F-2C-005 |
| AUD-K20 | Custom messageHandler (justified, alt documented) | **Keep** | High | Logging | 2C-F-2C-006, UW-06 |
| AUD-K21 | Leaked handler mutex | **Keep** | High | Logging | 2C-F-2C-012 |
| AUD-K22 | app.env SSOT fail-fast | **Keep** | High | Build | 2C-F-2C-013, UW-11/12 |
| AUD-K23 | Smoke-test path (bootstrap validation) | **Keep** | High | Lifecycle | 2C-F-2C-014 |
| AUD-K24 | MV* feature convention | **Keep** | High | Feature | 2D-F-201 |
| AUD-K25 | Auto-connect slot naming contract | **Keep** | High | Feature/Qt | 2D-F-204 |
| AUD-K26 | EventSystem header-as-contract (AI-ergonomics) | **Keep** | High | API | 2D-F-206 |
| AUD-K27 | generate.sh scaffold + CI lock | **Keep** | High | CodeGen | 2D-F-207, UW-16 |
| AUD-K28 | common.h always-emitted (when types exist) | **Keep** | Med | Feature | 2D-F-208 |
| AUD-K29 | String boundary policy | **Keep** | High | API | 2D-F-220 |
| AUD-K30 | Composition root + LifecycleTest locks | **Keep** | High | Composition | 2D-F-221 |
| AUD-K31 | Feature test suite (public API, no friends) | **Keep** | High | Testing | 2D-F-223 |
| AUD-K32 | app.env pipeline (scripts + Conan + CMake) | **Keep** | High | Build | 2E-F-2E-001 |
| AUD-K33 | Dependency visibility (fmt PRIVATE, cxxopts exe-only, Widgets PUBLIC) | **Keep** | High | Build | 2E-F-2E-002 |
| AUD-K34 | QT_QPA_PLATFORM offscreen before-build export | **Keep** | High | Build/Testing | 2E-F-2E-003 |
| AUD-K35 | AUTORCC ON + hex-scan canary | **Keep** | High | Build | 2E-F-2E-004, UW-20 |
| AUD-K36 | aqtinstall git-pin + exit criteria | **Keep** | High | Build | 2E-F-2E-005, UW-14 |
| AUD-K37 | install() exe-only (app template scope) | **Keep** | High | Build | 2E-F-2E-011 |
| AUD-K38 | --smoke-test packaging contract | **Keep** | High | Build | 2E-F-2E-013 |
| AUD-K39 | Heuristic boundary script (class B, honestly documented) | **Keep** | High | Build/Architecture | 2E-F-2E-015, UW-07 |
| AUD-K40 | Tests CMake structure (UnitTests, custom main, no gtest_main) | **Keep** | High | Testing/Build | 2E-F-2E-019 |
| AUD-K41 | Test-mode isolation contract | **Keep** | High | Testing | 2F-F-201 |
| AUD-K42 | Custom tests/main.cpp bootstrap | **Keep** | High | Testing | 2F-F-202 |
| AUD-K43 | Public-API testing (no friend classes) | **Keep** | High | Testing | 2F-F-203 |
| AUD-K44 | MemoryPersistenceProvider double + fail-injection | **Keep** | High | Testing | 2F-F-204 |
| AUD-K45 | Custom abstractions improve testability (seams earn rent) | **Keep** | High | Testing/Architecture | 2F-F-205 |
| AUD-K46 | EventSystem performance LOW (zero production publishers) | **Keep** | High | Performance | 2F-F-211 |
| AUD-K47 | Sync GUI-thread persistence non-goal + Async Policy | **Keep** | High | Performance/Concurrency | 2F-F-212 |
| AUD-K48 | Test suite as executable architecture spec | **Keep** | High | Testing | 2F-F-214 |

**Register totals:** Significant 4 · Improvement 23 · Opinion 11 · Keep 48 (canonical; angle-specific agent Keeps fold into these IDs with attribution preserved).

---

## D. Un-Workaround Registry

From 2G + verification adjustments. **Zero historical (D) mechanisms; mechanisms are A or B** (plus one C confirmed, one E-class *guard defect* that is not a custom mechanism).

### Summary table

| ID | Mechanism | Class | Conf. | Recommendation |
|---|---|---|---|---|
| UW-01 | EventSystem typed bus (BusRegistry, free-fn subscribe) | B | High | Keep (AUD-K01) |
| UW-02 | QVariant transport on EventDispatcherBase | **A** | High | Keep — moc limitation current in Qt 6.11 (AUD-K02) |
| UW-03 | PersistenceResult / PersistenceError | B | High | Keep; migrate on C++23 (AUD-K11) |
| UW-04 | IPersistenceProvider vs QSettings | B | High | Keep — typed errors + test seam (AUD-K11) |
| UW-05 | ServiceRegistry registerAll/unregisterAll | B | Medium | Keep (AUD-K16/K18) |
| UW-06 | Logging categories + custom messageHandler | B | High | Keep (AUD-K19/K20/K21) |
| UW-07 | Boundary grep script | B | Medium | Keep; multi-target later (AUD-K39) |
| UW-08 | Dual AppMainWindow ctors | B | High | Keep (AUD-K10) |
| UW-09 | m_models QList&lt;IModel*&gt; | **B (list) / E (assert)** | High | **Keep list**; fix/remove assert — see qualification |
| UW-10 | QPointer presenter null-guards | B | Medium | Keep — pedagogical value (AUD-203) |
| UW-11 | apply_app_metadata compile-def SSOT | B | High | Keep (AUD-K22) |
| UW-12 | app.env → env.sh → Conan → CMake pipeline | B | Medium | Keep; CMakePresets optional later (AUD-K32) |
| UW-13 | pipenv for Python tooling | C → **Improvement/Opinion** | Medium | Tooling modernization (uv), not a Qt un-workaround — see qualification |
| UW-14 | aqtinstall git-commit pin | B | High | Keep until PyPI ≥3.4.0 (AUD-K36) |
| UW-15 | py7zr → external 7-Zip | **A** | High | Keep — still required; **not D** (2E's D was wrong) |
| UW-16 | generate.sh MV* scaffold | B | High | Keep (AUD-K27) |
| UW-17 | Theme QSS + Windows Fusion + colorScheme() | B | High | Keep — Qt 6.5+ API, not legacy hack |
| UW-18 | Always-emitted {name}common.h | C | Low | Simplify optionally (AUD-205) |
| UW-19 | Static Subscription storage | B | Medium | Keep — storage ≠ lifecycle (AUD-K16) |
| UW-20 | CMAKE_AUTORCC ON | B | High | Keep + hex-scan (AUD-K35) |
| UW-21 | Subscription RAII over Connection | B | High | Keep (AUD-K04) |
| UW-22 | cxxopts/fmt Conan demos | B | High | Keep — intentional pedagogy |
| UW-23 | GLOB_RECURSE CONFIGURE_DEPENDS | B (glob) / C (fat-lib context) | Low/Med | Complementary to AUD-107; keep at template scale |
| UW-24 | EventSystem GUI-thread-only policy | B | High | Keep — deliberate single-thread policy, not a Qt workaround (AUD-K03) |
| UW-25 | Dual unregisterAll (explicit + qScopeGuard) | B | High | Keep (AUD-K16) |

**Counts after verification:** A=2 (UW-02, UW-15), B=20 (UW-09 qualified: list B), C=1 confirmed (UW-18; UW-13 reclassified to tooling), **D=0**, E=1 implicit (m_models **assert** guard per 2D — not a UW entry; it is a guard defect).

### Qualifications

**UW-09 qualification (critical):** The 2G entry text states the assert "catches forgotten appends." **That sentence is wrong and is not carried into this report.** Code at `AppMainWindow.cpp:57-62` hardcodes `findChildren<CounterModel*>() + findChildren<AppLogModel*>()`. Forgotten append of a **new** feature model → census stays 2, list stays 2 → assert passes (silent persistence loss). Correct append → census 2, list 3 → assert **false-positives** unless census is hand-edited. 2D-F-210 / 2B-F-2B-007 are correct; 2G appears to have echoed AGENTS.md wording without checking the extension case — the one place AGENTS.md was adopted uncritically in this audit. **List = B Keep; assert = E Significant (AUD-002).**

**UW-13 reclassification:** A–E rubric is about custom mechanisms vs canonical Qt/C++ facilities. pipenv is a third-party ecosystem tool choice, not a custom mechanism vs a Qt/C++ canonical. "C — redundant abstraction" doesn't map cleanly; better as Improvement/Opinion (AUD-112 covers --deploy parity). uv observation stands as tooling modernization.

**UW-15 / 2E class D correction:** 2E-F-2E-005 classified 7-Zip workaround as D. **2G is right:** py7zr bug still affects pinned aqt (Pipfile git pin; install-qt.sh workaround present because upstream not fixed). Class **A** — no better in-repo alternative until upstream fixes. Exit criteria: PyPI aqtinstall ≥3.4.0.


### Full blocks for the most important entries

---

#### UW-01 / AUD-K01 — EventSystem typed pub/sub bus (class B)

**Current workaround:** Hand-rolled typed event bus in `EventSystem.hpp` — BusRegistry function-local static, EventDispatcher&lt;T&gt; via QVariant on non-template base, Subscription RAII, free-fn + QObject subscribe, always-queued delivery, GUI-thread policy, aboutToQuit guards.

**Original problem:** Cross-component decoupled domain events without wiring every pair at composition root; free-function service handlers; teaching demonstration of event-bus semantics.

**Does the problem still exist?** Yes. Qt 6 has no built-in typed pub/sub for free-function subscribers. Qt signals require a QObject sender; a "bus QObject" with QVariant signal is a known pattern (EventDispatcherBase's core), but Qt does not package typed publish/subscribe, RAII free-fn handles, GUI-thread policy, or quit-guard lifecycle. Feature-internal data correctly uses Qt signals. No production `events::publish` except tests — intentional pedagogy, not unfinished feature.

**Canonical solution:** Qt signals/slots on feature QObjects; minimal `EventBus : QObject { void event(QVariant); }` + typed wrappers; Boost.Signals2 (not in deps).

**Why canonical isn't sufficient:** Free-fn `subscribe` returning move-only `[[nodiscard]] Subscription` is something `QMetaObject::Connection` does not provide (Connection does not auto-disconnect on destruction). Always-queued delivery + mid-dispatch deferral + recursive-publish queuing are enforced semantics locked by EventsTest. A simpler bus QObject would lose the typed API, Subscription RAII, and documented contract. Class B: additional semantics deliberate and tested. **The bus is the product** for this template's stated purpose.

**Recommendation:** Keep. Do not "simplify" away QVariant transport or the typed facade. Derived apps that never need cross-component events can ignore EventSystem without rewriting features.

**Migration complexity / risk:** High / High (lifetime/ordering contract load-bearing).

---

#### UW-02 / AUD-K02 — QVariant transport on non-template EventDispatcherBase (class A)

**Current workaround:** `EventDispatcherBase` (Q_OBJECT) carries `void eventPublished(const QVariant &)`; `EventDispatcher<T>` (no Q_OBJECT) emits via `QVariant::fromValue`; runtime meta-type check on delivery.

**Original problem:** Qt moc does not allow `Q_OBJECT` in class templates, so a per-type QObject with a typed signal cannot be declared as a template.

**Does the problem still exist?** **Yes.** Official Qt 6.11 moc docs: "The main problem is that class templates cannot have the Q_OBJECT macro." Still unsupported; "not a high priority" for Qt to fix. Not folklore.

**Canonical solution:** Non-template base + QVariant signal + typed facade (what the repo does); or one QObject per concrete event type (doesn't scale); or PMF-only connections.

**Why canonical isn't sufficient:** Repo's pattern is the standard workaround recommended by Qt's own limitation text. Runtime type check is defense-in-depth at the type-erasure boundary; primary protection is typed public API + private dispatcher. Not removable without abandoning typed bus events. **Class A — no reasonable canonical mechanism exists while moc limit holds.**

**Recommendation:** Keep. Do not replace with `EventDispatcher<T> : QObject { signals: void published(T); }` — it will not build.

**Migration complexity / risk:** N/A (cannot migrate).

---

#### UW-03 / AUD-K11 — PersistenceResult / PersistenceError (class B)

**Current workaround:** Custom `PersistenceResult<T>` (std::variant&lt;T, PersistenceError&gt;) + void specialization (std::optional&lt;PersistenceError&gt;), hasValue/hasError accessors, assert+abort on misuse; four-value PersistenceError enum.

**Original problem:** Typed error results from load/save without exceptions; explicit first-run vs operational failure distinction.

**Does the problem still exist?** Yes: CMake pins C++20 (`CMakeLists.txt:16-17`). `std::expected` is C++23. No Qt type provides this contract.

**Canonical solution:** `std::expected<T, PersistenceError>` (C++23); `std::optional<T>` (loses taxonomy); QSettings::Status / exceptions (rejected by policy).

**Why canonical isn't sufficient:** On C++20 there is no standard expected. Custom type is a deliberate, documented analogue. Misuse policy (assert/abort) stricter than std::expected::value() misuse. Taxonomy (NotFound/IoError/InvalidData/CommitError) load-bearing at provider boundary, locked by PersistenceTest on real disk. Optional would collapse NotFound with operational errors. Exceptions explicit non-choice.

**Recommendation:** Keep on C++20. On C++23 adoption, migrate both specializations to std::expected in a dedicated change; do not delete the type while on C++20.

**Migration complexity / risk:** Low (C++23) / N/A now.

---

#### UW-04 — IPersistenceProvider + File/Memory providers vs QSettings (class B)

**Current workaround:** Abstract IPersistenceProvider (loadState/saveState → PersistenceResult); FilePersistenceProvider (AppDataLocation JSON + QSaveFile + typed errors + all persistence logging); MemoryPersistenceProvider test double with fail-injection; models take IPersistenceProvider&.

**Original problem:** Injectable feature-state persistence with typed errors, testability, separation from QSettings chrome.

**Does the problem still exist?** Yes. QSettings stores QVariant key-values for GUI chrome; no NotFound/IoError/InvalidData/CommitError taxonomy at a provider API boundary, nor injectable seam for failure-injection without wrapping.

**Canonical solution:** QSettings for chrome (already used); QJsonDocument+QSaveFile directly in models; or QSettings for everything.

**Why canonical isn't sufficient:** Chrome via QSettings correct and separate. Feature state via provider interface enables MemoryPersistenceProvider + LifecycleTest injection + provider-only diagnostics. QSaveFile usage **is** Qt-native atomic write — custom part is provider interface + result type, not reinvented file I/O.

**Recommendation:** Keep. Do not replace with raw QSettings for feature state.

---

#### UW-05 / UW-19 / UW-25 / AUD-K16 — ServiceRegistry + static storage + dual unregister (class B)

**Current workaround:** Free-function registerAll/unregisterAll; static `events::Subscription` (storage only); called from main() on normal, smoke, and exception paths; qScopeGuard backup.

**Original problem:** Visible, process-level service subscription lifecycle with deterministic teardown independent of static destruction order.

**Does the problem still exist?** Partially. Qt alternative: QObject service host parented to QCoreApplication — viable for QObject services. Stock service is a **free function**, which does not fit QObject ownership without a wrapper QObject whose only job is to hold connections.

**Canonical solution:** `class ServiceHost : QObject` parented to app; Qt plugin system (overkill); or nothing.

**Why canonical isn't sufficient:** Explicit API pedagogical (teaches nodiscard Subscription + lifecycle). Idempotent unregister + qScopeGuard covers exception paths. QObject host more Qt-idiomatic but adds a class for one demo subscription. Static storage explicitly **not** the lifecycle mechanism — they rejected the "statics clean up themselves" myth. Static-init registration would fire before QApplication exists — EventSystem guards reject subscribe; failure invisible to void registerAll. For a *template*, showing the lifecycle is the product.

**Recommendation:** Keep. Preserve: register only after QApplication; unregister on every path after register; [[nodiscard]] at subscribe boundary; static storage documented as storage-only. If app grows many services, consider QObject ServiceHost.

---

#### UW-08 / AUD-K10 — Dual AppMainWindow constructors (class B)

**Current workaround:** `AppMainWindow(QWidget*)` constructs FilePersistenceProvider(this); `AppMainWindow(IPersistenceProvider&, QWidget*)` borrows caller-owned provider (must outlive; not reparented). Shared finishConstruction().

**Original problem:** Encode persistence-seam ownership mode in the type system; avoid dual-mode nullable pointer.

**Does the problem still exist?** Yes as a design need (tests need injection). Alternatives: nullable pointer (rejected — weaker); setter injection (ordering hazard); factory functions (more API surface).

**Canonical solution:** Nullable pointer param or setter; or always-owning provider with test-only subclass factory.

**Why canonical isn't sufficient:** Two ctors make ownership mode unrepresentable-as-error: you cannot construct with "maybe own." Injected non-reparenting explicit. Qt apps commonly use ctor injection; Qt has no DI container. Small, clear pattern.

**Recommendation:** Keep. Do not "simplify" into a nullable pointer or optional provider.

---

#### Abbreviated remaining entries (summary-table verdicts apply; one-line rationale)

| ID | Mechanism | One-line rationale for class |
|---|---|---|
| UW-07 | Boundary grep script | No stock tool implements these six rules; multi-lib CMake is "real" fix but disproportionate at template scale; script self-documents evasion and is honestly labeled evidence-not-proof. UNIX-only CTest registration; build.sh always runs it. |
| UW-10 | QPointer null-guards | Qt-canonical mechanism (not a workaround). Signal-path redundancy noted; guards matter for mid-session teardown and template pedagogy. Residual: ctor initial sync not null-guarded. |
| UW-11/12 | app.env SSOT + pipeline | No C++ mechanism replaces build-time SSOT flowing into compile definitions + shell-sourceable file for scripts/packaging. CMakePresets (not present) would not replace aqt/pipenv/smoke paths. Minor caveat: QT_CMAKE_DIR silent ./Qt fallback. |
| UW-14/15 | aqt git-pin + 7-Zip | PyPI 3.3.0 cannot install Qt 6.11+ on Windows; git pin documented bridge with exit criteria (PyPI ≥3.4.0). 7-Zip workaround still required — py7zr bug affects pinned aqt — **class A**, not D. |
| UW-16 | generate.sh scaffold | Template product code, not a Qt workaround. Qt Creator wizards do not emit this contract. CI test-generator locks output. |
| UW-17 | Theme QSS + Fusion | QStyleHints::colorScheme() is Qt 6.5+ modern API; Windows Fusion force still required for widgets dark QSS. custom.qss is simple extension point; empty platform QSS = Qt default by design. |
| UW-18 | common.h always-emit | C, low impact. Rule 1 makes common.h correct place for widget-visible shared types (LogDelta); empty stubs are acceptable template cost. |
| UW-19 | Static Subscription storage | Storage ≠ lifecycle distinction is the key insight; explicit unregisterAll is the mechanism; qScopeGuard backup on all main() paths. |
| UW-20 | CMAKE_AUTORCC ON | qt_standard_project_setup does NOT enable AUTORCC (Qt 6.11 docs confirmed). Silent no-op footgun is real; comment + hex-scan test are correct responses. Both AUTORCC and qt_add_resources are Qt-blessed; AUTORCC chosen for convenience. |
| UW-21 | Subscription RAII | QMetaObject::Connection does not RAII-disconnect — wrapper adds real value; [[nodiscard]] compile-time discipline Qt does not enforce for connections. |
| UW-22 | cxxopts/fmt demos | Intentional Conan pedagogy documented in AGENTS.md; Qt/std alternatives exist but removal deletes demo value. |
| UW-23 | GLOB_RECURSE CONFIGURE_DEPENDS | B for glob mechanism (CONFIGURE_DEPENDS is supported CMake mitigation); C only in fat-lib context. Complementary to AUD-107; keep at template scale. |
| UW-24 | GUI-thread policy | Deliberate single-thread simplification, not a Qt workaround (queued connections make cross-thread delivery easier). Async Policy escalation path documents when to revisit. |
| UW-25 | Dual unregisterAll | Explicit calls + qScopeGuard; idempotent; documented intentional redundancy for readability + exception safety. |

---

## E. Architecture Debt Register

Architectural (not code) debt:

| Debt item | Where it bites | Finding |
|---|---|---|
| **Broken extension assert** | First added feature in debug build either false-positives or gets no protection; AGENTS.md E10/H9 overstate | AUD-002 |
| **Convention-only provider lifetime (default path)** | Future model dtor doing provider I/O = UB; no type-system or test enforcement | AUD-001 |
| **Composition silent-failure checklist** | Four extension failure points documented but non-binding; test-generator never wires AppMainWindow; agents that skip AGENTS.md get no protection | AUD-104, AUD-119 |
| **Grep-only layering** | Single fat `_lib` + PUBLIC src/ means CMake graph cannot express layering; heuristic script evadable; no structural backstop | AUD-107 |
| **Windows smoke path inference** | Dev-loop smoke likely silent no-op under pinned Ninja; PR CI on Windows never launches production binary | AUD-003 |
| **Dockerfile/profile drift** | Dev-container builds may produce gcc-15-labeled package-ids while compiling differently; CI echo-only "validation" | AUD-108 |
| **Duplicated platform path logic** | Five scripts + conanfile independently encode binary location; drift already caused AUD-003 | AUD-109 |
| **Hardcoded autogen include path** | Qt/CMake layout change breaks all _lib consumers with confusing ui_*.h errors | AUD-110 |
| **Test gaps in lifetime-critical EventSystem paths** | aboutToQuit/quitFired_ + wrong-thread rejection untested; shutdown UAF regression would ship silently | AUD-004, AUD-101 |
| **QSettings chrome channel has no failure observation** | Geometry/state save/load can fail silently; asymmetric with feature-state channel's full taxonomy | AUD-103 |
| **Config-channel discoverability** | QSettings-vs-provider split lives in AGENTS.md, weakly in headers; agents misroute chrome vs feature state | AUD-118 |
| **Vocabulary collisions** | "Model" ≠ QAbstractItemModel; "Service" ≠ general DI — wrong-scaffold risk for agents | AUD-201, AUD-117 |
| **Thread-guard asymmetry** | EventSystem runtime-guards misuse; models/providers document-only — inconsistent misuse-resistance standard | AUD-115 |
| **AppImage unpinned downloads** | Only unpinned corner in an otherwise locked pipeline; supply-chain + reproducibility gap | AUD-111 |
| **Latent multi-QApplication dead bus** | quitHookInstalled_/quitFired_ never re-arm; unreachable today but contradicts "process-level" documentation if exercised | AUD-123 |

---

## F. Keep List

Explicit keep verdicts. These should **not** change without compelling technical reason.

| Keep ID | Item | Class | Why justified |
|---|---|---|---|
| AUD-K01 | **EventSystem as typed pub/sub scaffolding** | B | Qt provides no free-fn typed pub/sub with Subscription RAII, all-builds GUI-thread policy, or quit lifecycle. Zero production publishers is documented pedagogy. **Do not delete because publishers are absent.** |
| AUD-K02 | **QVariant transport on EventDispatcherBase** | **A** | Qt 6.11 moc: class templates cannot have Q_OBJECT — current limitation. Typed signal on EventDispatcher&lt;T&gt; will not build. |
| AUD-K03 | **Queued delivery + GUI-thread policy** | B | Deliberate semantic lock stronger than Qt AutoConnection. All-builds guards turn violations into visible qCritical lines. |
| AUD-K04 | **Subscription RAII + [[nodiscard]]** | B | Qt Connection does not disconnect on destruction; wrapper adds RAII + [[nodiscard]]. Discard=disconnect is intended contract for free-fn handlers. |
| AUD-K05 | **Free-fn subscribe + QCoreApplication context** | B | Qt context-bound functor connections are canonical for application-lifetime free-fn handlers. Explicit unregisterAll is not static-dtor reliance. |
| AUD-K06 | **BusRegistry lifetime design** | B | Function-local static is simplest correct form for process-level typed bus. Parenting dispatchers to QCoreApplication + clearing non-owning map on aboutToQuit = correct ownership split: Qt deletes objects; registry only forgets them. quitFired_ prevents post-quit recreation. |
| AUD-K07 | **Logging/EventSystem separation** | N/A | Qt logging is canonical. Routing diagnostic logs through pub/sub would invert dependency (logging must work before/during/without EventSystem; bus itself logs via qCCritical). Enforced one-way events→logging; boundary Rule 2. |
| AUD-K08 | **Demo-only DemoLogEvent** | N/A | Intentional pedagogy, consistently documented. Demo* naming prevents code shape implying live production flow. |
| AUD-K09 | **No filter/transform/error channel** | N/A | Qt signals/slots also have no predicates/transform hooks/error propagation. Bus is at parity with canonical mechanism. Adding these would diverge from Qt mental model without template use case. |
| AUD-K10 | **Dual-ctor persistence seam** | B | Two ctors make ownership mode unrepresentable-as-error: cannot construct with "maybe own." Injected non-reparenting explicit. Tests need injection; production needs owned default. Qt has no DI container. **Do not simplify into nullable pointer.** |
| AUD-K11 | **PersistenceResult + taxonomy + provider-owned diagnostics** | B | C++20 pin makes PersistenceResult the right expected-analogue. Four-value taxonomy load-bearing, locked by PersistenceTest on real disk. Void loadState / result saveState asymmetry intentional. |
| AUD-K12 | **closeEvent log-and-continue + stack window** | N/A | hide() first is real Qt UX contract. m_models loop is only polymorphic shutdown-persistence path — IModel deliberately not a QObject. WA_DeleteOnClose would double-destroy stack window. Blocking close on persistence failure worse for GUI app. |
| AUD-K13 | **FilePersistenceProvider QSaveFile + APP_ID keys** | N/A | QSaveFile is canonical Qt atomic-replace; comments honest about limits (no fsync surfaced). Distinguishing NotFound from IoError/InvalidData makes first-run vs corruption testable. APP_ID keys tie persistence namespacing to app.env SSOT without inventing key registry. |
| AUD-K14 | **m_models list (NOT the assert)** | B | IModel deliberately not a QObject. List is minimal mechanism for polymorphic shutdown persistence without marker base, registration macro, or parallel save API. **List = Keep/B; assert = AUD-002 Significant/E.** |
| AUD-K15 | **Mechanism census (no custom lifetime framework)** | N/A | Mechanism mix maps to object semantics: parent-child for QObjects; unique_ptr for Ui::*; QPointer for borrow; ctor overload for persistence seam; explicit RAII for non-QObject services. |
| AUD-K16 | **Service registration (explicit, no macros) + dual unregister + qScopeGuard** | B | Static-init registration would fire before QApplication — failure invisible to void registerAll. For a *template*, showing the lifecycle is the product. Static storage ≠ lifecycle mechanism. |
| AUD-K17 | **Theme/translator lifetime** | N/A | Theme holds no owned state — "state" is qApp's stylesheet. Translator declaration order + guard ordering is textbook correct Qt i18n lifetime pattern. |
| AUD-K18 | **Explicit service registration (no macros)** | B | No REGISTER_SERVICE macros anywhere; sole assignment site ServiceRegistry.cpp. Auto-registration would hide register/unregister pairing. |
| AUD-K19 | **Logging categories + configureLogLevel** | N/A | QLoggingCategory + setFilterRules *is* Qt-native. Thin pure-function token mapper (13 lines) keeps CLI in main.cpp while mapping lives with categories. Wildcard rules mean new categories inherit filtering for free. |
| AUD-K20 | **Custom messageHandler** | B | qSetMessagePattern can format but cannot restore stdout/stderr split or make fatal abort visible in application code — claimed extra semantics. Handler uses official extension point, not private wrapper. In-code rationale documents qSetMessagePattern equivalent. |
| AUD-K21 | **Leaked handler mutex** | B | Standard defensive pattern against static-destruction-order deadlock: function-static mutex without leak could be destroyed while later-destroying static object still logs. Non-recursive constraint honest (handler must not call qC*). |
| AUD-K22 | **app.env SSOT fail-fast + apply_app_metadata** | B | No C++ mechanism replaces build-time SSOT flowing into compile definitions. Fail-fast prevents undefined identifiers or silent wrong identity. |
| AUD-K23 | **Smoke-test path** | N/A | Job is bootstrap-path validation: CLI + logging + app + registerAll + window graph + theme + translator, without display. Deliberately shares production startup sequence. Persistence-write verification correctly left to LifecycleTest — separation of concerns correct. |
| AUD-K24 | **MV* feature convention** | B | Implemented convention matches documented one almost exactly. Layering real. Presenter pattern is the seam where persistence-restored state reaches view. |
| AUD-K25 | **Auto-connect slot naming** | N/A | Both Qt-native choices. Repo choice consistent, verified in reference features + generator comments; silent-failure mode documented. |
| AUD-K26 | **EventSystem header-as-contract** | B | Strongest single artifact for AI-agent ergonomics: agent reading only EventSystem.hpp can answer subscribe/publish/ownership/threading/teardown correctly without reverse-engineering BusRegistry. |
| AUD-K27 | **generate.sh scaffold + CI lock** | B | Template product code, not workaround for Qt gap. Qt Creator wizards do not emit this MV*/persistence/presenter contract. Generated code shape matches Counter/AppLog on all load-bearing patterns; test-generator.sh locks conventions in CI. |
| AUD-K28 | **common.h always-emitted (when types exist)** | N/A | When shared types exist (LogDelta), header is load-bearing; boundary Rule 1 forbids widgets including model headers — common.h is correct place. Empty stubs acceptable template cost. |
| AUD-K29 | **String boundary policy** | N/A | Implemented consistently (one conversion site per boundary crossing), documented in AGENTS.md. Mixed QString/std::string APIs without boundary rule are classic template footgun; this template defines the rule and follows it. |
| AUD-K30 | **Composition root + LifecycleTest locks** | B | Code-built central layout; dual finishConstruction path; LifecycleTest locks graph/stack/closeEvent/injection end-to-end. |
| AUD-K31 | **Feature test suite (public API, no friends)** | N/A | grep friend: zero matches; test-generator.sh forbids friend class. Production headers stay free of test-only surface. Ownership observability via QObject tree without exposing private BusRegistry API. |
| AUD-K32 | **app.env pipeline (scripts + Conan + CMake)** | B | Scripts encode project's opinion; each layer has a job documented in AGENTS.md. CMakePresets optional later — do not remove app.env. |
| AUD-K33 | **Dependency visibility (fmt PRIVATE, cxxopts exe-only, Widgets PUBLIC)** | N/A | CMake visibility used correctly: fmt PRIVATE (logging.h has no fmt); cxxopts only in main.cpp → exe-only; Widgets PUBLIC (_lib public headers include QWidget/QMainWindow types). |
| AUD-K34 | **QT_QPA_PLATFORM offscreen before-build export** | B | Required because POST_BUILD test discovery runs test binary inside build step; GUI binary without display plugin fails discovery. Exporting before build is simplest correct placement. |
| AUD-K35 | **AUTORCC ON + hex-scan canary** | B | Guards documented silent failure native test surface cannot catch (tests embed resources independently; can pass even if production exe lost resources). |
| AUD-K36 | **aqtinstall git-pin + exit criteria** | B | PyPI 3.3.0 cannot install Qt 6.11+ on Windows; git pin temporary, documented bridge with explicit exit condition. |
| AUD-K37 | **install() exe-only (app template scope)** | B | Template purpose is application generator, not library distribution. Shipping without package-config avoids obligations template never exercises. |
| AUD-K38 | **--smoke-test packaging contract** | B | No native Qt/CMake mechanism launches fully deployed artifact headless; failure modes it catches (missing Qt plugins/DLLs after deploytools, broken rpaths) exactly what unit tests in build tree cannot see. |
| AUD-K39 | **Heuristic boundary script (class B, honestly documented)** | B | ~137 lines portable bash, readable by AI agents and humans, fails fast before 10-minute Conan build. Self-documents residual limitations. Native alternatives (multi-lib CMake, clang-tidy) disproportionate for single-app template. |
| AUD-K40 | **Tests CMake structure (UnitTests, custom main, no gtest_main)** | B | Hardcoded UnitTests honest for template; custom tests/main.cpp load-bearing (test mode + QSettings redirect — gtest_main would skip bootstrap). |
| AUD-K41 | **Test-mode isolation contract** | A | Without test-mode + QSettings redirect, LifecycleTest and PersistenceTest would write into developer's real AppDataLocation and organization settings. QSettings explicitly NOT covered by Qt test mode — extra redirect load-bearing, not redundant. |
| AUD-K42 | **Custom tests/main.cpp bootstrap** | N/A | Single QApplication must exist for whole binary: EventSystem guards, services registerAll, theme tests, GUI feature tests all require one app instance. gtest_main would hide bootstrap; isolation order impossible to guarantee process-wide. |
| AUD-K43 | **Public-API testing (no friend classes)** | N/A | Production headers stay free of test-only surface. findChild works because Qt object names are part of UI contract. |
| AUD-K44 | **MemoryPersistenceProvider double + fail-injection** | A | Custom IPersistenceProvider has no canonical Qt in-memory substitute; without double, model error-path tests need real files + permission games. |
| AUD-K45 | **Custom abstractions improve testability (seams earn rent)** | B | IPersistenceProvider enables model tests without disk + LifecycleTest injection. Dual ctors encode ownership in type system. m_models enables e2e closeEvent test. None are test-only hacks. |
| AUD-K46 | **EventSystem performance LOW (zero production publishers)** | N/A | Zero production publish sites; mutex uncontended; no per-sub wrapper QObject. Optimizing now = premature micro-optimization. |
| AUD-K47 | **Sync GUI-thread persistence non-goal + Async Policy** | B | Justified non-goal: state payloads small JSON; Async Policy documents escalation path; shutdown never blocks close on persistence failure. |
| AUD-K48 | **Test suite as executable architecture spec** | N/A | Suite locks core ownership/persistence/service contracts easy to break when adding features. Core contracts genuinely locked, not ceremonial. |

---

## G. Proposed Target Architecture

Significant changes warranted, but target is **keep the architecture; fix the four Significant issues + high-value Improvements** — not a redesign. Core decisions (EventSystem, persistence seam, dual ctors, explicit services, app.env SSOT, MV* convention, single-thread policy) remain as-is.

### How pieces fit together

```text
main()
  [AUD-001 Option B: stack provider, inject into window]
  OR keep default ctor + document convention-only lifetime
  services::registerAll() + qScopeGuard (unchanged)
  AppMainWindow(provider, ...)
    └─ constructFeatures() [AUD-104 Option A/B — once]
         ├─ registerModel(IModel*) [AUD-002 Option B]
         │    └─ m_models append + count tracking
         └─ attach(model, widget) [AUD-104 Option B, optional]
              └─ m_models append + mainLayout->addWidget
    finishConstruction():
      QSettings restore + status observe [AUD-103]
      layout build (unchanged)
      NO hardcoded findChildren census [AUD-002]
  [AUD-003: scripts resolve APP_BIN via shared helper]
    scripts/lib/resolve-app-path.sh
      try single-config path first, multi-config fallback
      used by build.sh, run.sh, configure-vscode.sh, packaging
  --smoke-test works on Windows under Ninja (verified)
  closeEvent: QSettings + status [AUD-103] + m_models loop
    (unchanged log-and-continue policy)

EventSystem:
  quit hooks re-armed per QCoreApplication [AUD-123 Option A]
  OR documented single-app assumption [AUD-123 Option B]
  + quit lifecycle tests [AUD-004]
  + wrong-thread tests [AUD-101]
  + cross-subscriber order test [AUD-120]
  + optional: QObject subscribe returns Subscription [AUD-105 Option A]

Docs (AGENTS.md + headers):
  E10/H9 wording matches whatever m_models guard remains [AUD-002]
  "Model" ≠ QAbstractItemModel [AUD-201]
  ServiceRegistry scope = EventSystem free-fn only [AUD-117]
  Config channels in AppMainWindow.h + IPersistenceProvider.h [AUD-118]
  Provider lifetime convention-only on default path [AUD-001]

Tests:
  LoggingTest behavioral assert via handler capture [AUD-102]
  main.cpp exception-path coverage if cheap [AUD-114]
  MemoryPersistenceProvider isObject parity [AUD-113]
```

### What explicitly does NOT change

- EventSystem is **not** replaced. QVariant transport stays (class A — moc).
- Persistence is **not** rewritten. PersistenceResult + taxonomy + provider-owned diagnostics stay.
- No DI frameworks. Explicit service registration stays.
- No C++23 migration now. PersistenceResult → std::expected only when pin moves.
- Dual-ctor provider seam stays. No nullable-pointer "simplification."
- m_models **list** stays. Only the assert changes.
- Single fat `_lib` stays at template scale. No multi-target layering now.
- Sync GUI-thread persistence stays. Async Policy escalation path stays.

---

## H. Migration Plan

Ordered: correctness → lifetime/concurrency safety → obsolete workarounds → complexity → API → consistency → cosmetic. Independent vs Coordinated marked.

### Wave 1 — Documentation corrections (Independent, Low risk)

| Item | What changes | Finding |
|---|---|---|
| AGENTS.md E10/H9 wording | Replace "assert catches forgotten appends" / "matches constructed feature models" with language matching guard that remains (or "census of reference types only") | AUD-002 |
| Model vocabulary | AGENTS.md Feature Structure + IModel.h: "This Model is application state, not QAbstractItemModel. For item views prefer QAbstractListModel." | AUD-201 |
| Provider lifetime convention | AGENTS.md + generate.sh model-template comments: dtor rule convention-only on default path; injected path structural | AUD-001 (if Option A) |
| ServiceRegistry scope | ServiceRegistry.hpp header comment: EventSystem free-fn subscriptions only; no general DI | AUD-117 |
| Config channels | AppMainWindow.h + IPersistenceProvider.h short comments: chrome→QSettings, feature state→provider | AUD-118 |
| Thread-guard asymmetry | AGENTS.md Threading: models/providers documented-only vs EventSystem runtime-guarded | AUD-115 |

### Wave 2 — Fix m_models assert (Independent, Low risk)

| Option | What changes | Complexity |
|---|---|---|
| **Preferred: B** registration helper | Private `registerModel(IModel*)` in AppMainWindow; ctor wiring uses it; assert becomes tautological or compares against generated expected count; generate.sh checklist step becomes "call registerModel" | Low |
| Alternative: A** remove assert | Delete Q_ASSERT block; strengthen LifecycleTest with wired probe feature in test-generator.sh; update AGENTS.md | Low (assert removal) / Medium (CI wiring test) |

Also update generate.sh checklist either way. Cross-ref Wave 1 AGENTS.md wording.

### Wave 3 — EventSystem test gaps + quitFired_ re-arm (Independent, Low-Medium risk)

| Item | What changes | Finding |
|---|---|---|
| Quit lifecycle test | In-process or subprocess: after aboutToQuit / app instance gone, publish/subscribe don't deliver, don't crash; map entries not dereferenced after clear | AUD-004 |
| Wrong-thread tests | Worker thread publish + free-fn subscribe; assert no delivery + no crash; reuse ServiceRegistrationTest handler-probe for qCritical capture | AUD-101 |
| Cross-subscriber order test | Subscribe A then B; publish; expect order [A,B] | AUD-120 |
| quitFired_ re-arm OR document | Option A: re-arm hook per QCoreApplication instance + clear quitFired_ on new app. Option B: document "one QCoreApplication per process" + assert on recreation | AUD-123 |

### Wave 4 — Windows path helper unification (Independent, verify on Windows)

| Item | What changes | Finding |
|---|---|---|
| Shared path helper | `scripts/lib/resolve-app-path.sh` exporting APP_BIN; single-config first, multi-config fallback; fail loudly in CI when BUILD_TESTING=ON and exe missing | AUD-109 |
| build.sh / run.sh / configure-vscode.sh / packaging | Consume the helper | AUD-003, AUD-109 |
| conanfile.py DLL copy | Align DLL destination with actual layout or document | AUD-003 |
| **Verify on Windows** | Run build.sh --test ON on real Windows+Ninja machine; confirm smoke-test launches; treat AUD-003 as confirmed only after this | AUD-003 |

### Wave 5 — QSettings observation + LoggingTest behavioral assert (Independent, Low risk)

| Item | What changes | Finding |
|---|---|---|
| QSettings status | closeEvent: after setValue loop, check `settings.status()`; log warning on non-NoError; never block close | AUD-103 |
| LoggingTest | Handler-capture: configureLogLevel("debug") → probe qCDebug captured; "error" → not captured; unknown token leaves previous rules intact | AUD-102 |
| MemoryPersistenceProvider | Add `doc.isObject()` check for taxonomy parity | AUD-113 |

### Wave 6 — Composition ergonomics (Coordinated with generator)

| Item | What changes | Finding |
|---|---|---|
| constructFeatures() helper | Private method called from both ctors after provider bind; removes double-maintenance of wiring | AUD-104 |
| attach() helper (optional) | `attach(model, widget)` appends to m_models + adds widget to layout; generate.sh checklist shrinks | AUD-104, AUD-002 |
| Generator wiring template | generate.sh emits constructFeatures/attach usage pattern in checklist | AUD-104 |
| Generated test stub | Seed MemoryPersistenceProvider + construct MV triple + assert view shows restored state | AUD-119 |
| test-generator.sh | Behavioral checks beyond comment greps | AUD-119, AUD-106 |

### Wave 7 — Tooling hygiene (Independent, Low risk)

| Item | What changes | Finding |
|---|---|---|
| Dockerfile alignment | Install gcc-15 in Dockerfile to match conan/profiles/linux; CI guard comparing runner compiler vs profile | AUD-108 |
| AppImage pins | Pin appimagetool release tag + sha256; vendor excludelist | AUD-111 |
| pipenv --deploy parity | env.sh installPipenv uses --deploy to match CI, or document --skip-lock choice | AUD-112 |
| inno.iss hygiene | Emit to DIST_DIR + cleanup trap (already gitignored at .gitignore:110 — do not re-add) | AUD-122 |
| QSS list dedup | Derive tests/CMakeLists.txt QSS list from resources.qrc, or qt_add_library from qrc | AUD-121 |
| autogen path | Re-test generator expression in consumer context; if works, replace hardcode | AUD-110 |
| Optional: generator fidelity | Add logging include + instantiation line to widget template; reword test-stub findChild example | AUD-106 |
| Optional: AppLogWidget | setLogMessages clears then adds; presenter full-state refresh | AUD-116 |

### Wave 8 — Optional later (revisit only if requirements change)

| Item | When to revisit | Notes |
|---|---|---|
| CMakePresets.json | IDE users need presets without scripts | Parallel entry; do not remove app.env or scripts |
| Multi-target layering | Template evolves into larger application | Split _lib into core/events/features libs with PRIVATE includes; boundary script can remain as documentation |
| std::expected migration | C++23 pin adopted | Dedicated change; PersistenceResult stays until then |
| QObject subscribe handle | EventSystem API pass | Return Subscription from QObject overload too |
| pipenv → uv | CI times or onboarding pain material | Tooling modernization; no C++ behavior change |
| EventSystem cross-thread | Consumer app measures cross-thread event need | Async Policy escalation first; bus structure can support queued cross-thread delivery |
| ReusableWidget relocation | Teaching-artifact cleanup pass | Move to examples/ before deleting |

---

## Final Question (AUDIT.md §25)

> **If you were designing this template from scratch today, knowing everything discovered during this audit, which parts would you build differently — and which parts would you intentionally build exactly the same way?**

### Build differently

Genuine architectural lessons — things the evidence says would be designed differently on a fresh pass:

1. **Composition-root extension guard.** The m_models census assert actively misleads for the template's primary use case (adding features). From scratch: a registration helper or generated wiring test from day one — not a hardcoded two-type census that false-positives correct extension. The silent-failure checklist would be paired with a mechanical guard (attach/registerModel), not documentation alone.
2. **Provider lifetime encoding on the default path.** The convention-only sibling-children arrangement is safe today only because no model touches the provider in its dtor. From scratch: either inject from main() with guaranteed C++ stack destruction order (structural), or make the default path explicitly "test/demo path" and have production demonstrate the structural pattern. The dual-ctor seam stays; only the *default* path's ownership story changes.
3. **Platform binary-path resolution.** Five scripts independently encoding multi/single-config layout is what produced the Windows smoke-test gap. From scratch: one shared helper from the first script that needs it, with both layouts handled. Process lesson, not Qt architecture — but architectural because it affects the template's most-used surface.
4. **Vocabulary disambiguation.** "Model" colliding with QAbstractItemModel and "Service" colliding with general DI are cheap-to-fix discoverability failures avoided from scratch with one paragraph of documentation. Architecture not wrong; *naming surface presented to agents* is under-specified.
5. **Config-channel and thread-guard documentation in headers.** QSettings-vs-provider split and EventSystem-vs-models thread-guard asymmetry are real design facts living only in AGENTS.md. From scratch: those contracts appear at definition sites (headers), the way EventSystem.hpp already does — the strongest pattern in the repo.
6. **Test coverage for lifetime-critical EventSystem paths from day one.** aboutToQuit/quitFired_ and wrong-thread rejection are the two behavioral contracts most likely to regress silently. From scratch: those tests ship with the feature, not wait for an audit to flag the gap.

### Build exactly the same way

Intentionally identical — evidence says current design is correct; "from scratch today" is not permission to redesign:

1. **EventSystem as typed pub/sub scaffolding** — class B justified abstraction; zero production publishers is pedagogy, not a bug. QVariant transport (class A) cannot be built around under current moc.
2. **Always-queued delivery + all-builds GUI-thread policy** — deliberate semantic lock stronger than Qt AutoConnection; violations visible.
3. **Subscription RAII + [[nodiscard]] + QCoreApplication context for free-fn** — correct modern Qt pattern; explicit unregisterAll as lifecycle (not static-dtor reliance) is right discipline.
4. **Persistence three-layer model** — PersistenceResult + four-value taxonomy + provider-owned diagnostics + closeEvent log-and-continue. C++20 pin makes custom expected-analogue correct; std::expected is later migration, not design correction.
5. **Dual-ctor persistence seam** — ownership mode as type-level choice; injected provider not reparented. Alternative (nullable pointer) is misuse magnet.
6. **m_models list** (not the assert) — polymorphic shutdown persistence for non-QObject interface; minimal mechanism, no parallel save path.
7. **Explicit service registration, no macros** — static-init would fire before QApplication; for a template, showing the lifecycle is the product.
8. **app.env SSOT + fail-fast** — compile definitions flowing from one shell-sourceable file; no silent APP_* fallbacks.
9. **AUTORCC ON + hex-scan canary** — guards documented Qt CMake silent failure; both AUTORCC and test are correct responses.
10. **Test-mode isolation contract** — setTestModeEnabled + QSettings redirect before any fixture; load-bearing because Qt test mode does not cover QSettings.
11. **Public-API testing, no friend classes** — findChild + QObject tree observability; production headers stay clean.
12. **Heuristic boundary script, honestly documented** — green run is evidence, not proof; native alternatives disproportionate at template scale.
13. **Sync GUI-thread persistence + Async Policy escalation path** — justified non-goal; introducing workers without measured workload would be speculative complexity.
14. **String boundary policy** — QString in Qt/UI, std::string at generic bus payloads, one conversion per boundary crossing.
15. **MV* feature convention** — models load in ctor, widgets UI-only, presenters QPointer + initial sync; implemented convention matches documented one almost exactly.
16. **aqtinstall git-pin with documented exit criteria** — temporary bridge until PyPI release ≥3.4.0; not a hack without an end state.
17. **cxxopts/fmt as Conan demos** — intentional pedagogy; downstream apps may drop them.

### Distinguishing lessons from style

The six "build differently" items are **architectural process lessons**: guards that punish correct extension, lifetime conventions that are convention-only where structural encoding was available, duplicated script logic that drifts, vocabulary under-specified at definition sites, contracts that live only in prose, and test coverage deferred past the point where contracts were written. None say "EventSystem was a mistake," "persistence should be QSettings," or "the architecture is fighting Qt."

The "build exactly the same way" list is longer because the audit's dominant finding is that this template's custom machinery earns its rent. The Un-Workaround Registry found **zero historical (D) mechanisms**. The keep-dominant profile is not audit inflation — it is the correct read of a well-designed, unusually well-documented template whose remaining problems are about *extension safety, tooling drift, and test coverage*, not wrong architecture.

---

## Appendix: Claim Verification Roll-Up

AGENTS.md claims vs status vs evidence (VERIFICATION-REPORT §5 + CLAIMS-CHECKLIST). **No AGENTS.md claim was found to directly contradict the code.** Failures are overstatements, partial/operational gaps, honest descriptions of gaps.

| Claim (AGENTS.md area) | Status | Code evidence | Notes |
|---|---|---|---|
| **E10 / H9 — m_models assert "matches constructed feature models" / "catches forgotten appends"** | **Overstated** | `AppMainWindow.cpp:53-62` hardcodes `findChildren<CounterModel*>() + findChildren<AppLogModel*>()` | True only for stock two-feature composition. For new features: forgotten append NOT caught; correct append FALSE-POSITIVES unless census edited. generate.sh checklist omits census update. |
| **B15 / "provider must outlive models" as enforced invariant** | **Partial** (convention on default path) | Default ctor: provider + models sibling QObject children (`AppMainWindow.cpp:17-25`). Injected path: caller-owned, outliving structural (`:29-43`). Models have no custom dtors today. | Rule is documentation + "current models happen to have no dtors" on default path. No type-system or test enforcement for future model dtor touching provider. |
| **F9 — build.sh --test ON runs production --smoke-test "platform-aware path like run.sh"** | **Partial** (operational gap) | `build.sh:78` Windows path `${BUILD_DIR}/${CMAKE_BUILD_TYPE}/...` vs `conan/profiles/windows:11` Ninja single-config → exe at `${BUILD_DIR}/${APP_NAME}.exe`; missing path → `:88` WARNING + skip. windows-bundle.sh has multi/single-config fallback. | Mechanism claim code-true; **effective Windows coverage via build.sh alone not supported.** PR CI on Windows never launches production binary (bundle re-smoke only on main pushes). |
| **Dockerfile/profile alignment ("Keep profiles aligned with CI runners")** | **Partial** (instruction not followed) | Dockerfile `FROM ubuntu:22.04` + build-essential (gcc ~11); `conan/profiles/linux` compiler.version=15. CI toolchain step echoes only, never validates. | Dev-container builds may produce package-ids labeled gcc-15 while compiling differently. |
| **B18 — QSettings chrome channel "non-blocking by omission"** | **Verified as description of a real gap** | `AppMainWindow.cpp:64-66` restore without status; `:90-92` setValue without status()/sync() | Claim accurate; documenting gap doesn't close it. AUD-103 recommends observing QSettings::status(). |
| **Feature widget "UI only, no business logic"** | **Partial** | `AppLogWidget::handleLogChanged` re-encodes trim protocol (`delete takeItem(0)` if trimmed) — mirrors model trim bookkeeping | Not "business logic" (model owns trim policy) but not pure display either. `setLogMessages` name promises replacement, impl appends. |
| **A3 — cross-subscriber order follows Qt connection creation order** | **Partial** (documented, untested) | `EventsTest.cpp:175-187` count-only; per-connection FIFO locked `:593-610` | Honest documentation of untested Qt-derived property. Cheap test closes it (AUD-120). |
| **A10 / A14 / G2 — GUI-thread policy enforced at runtime in all builds** | **Verified in code** (behavioral test gap) | Guards at four EventSystem.hpp sites (qCCritical+no-op all builds; Q_ASSERT debug). EventsTest has zero QThread/std::thread. | Claim true in code; not locked behaviorally. Regression removing runtime guard (leaving debug assert only) ships silently in release (AUD-101). |
| **G6 — logging is thread-safe (QMutex in message handler)** | **Partial** | Leaked non-recursive QMutex at `logging.cpp:53`; handler must not call qC*. No multi-thread logging test. | Mutex present and correct; thread-safety verified by construction, not test. |
| **H13 — tests use public API + findChild; no friend test classes** | **Verified** | grep friend: zero matches in src/ and tests/; test-generator.sh forbids `friend class` in generated headers | |
| **J1–J6 — String Boundary Policy** | **Verified** | `DemoLogEvent.h:10` std::string; conversion once at `AppLogPresenter.cpp:32` and `DemoConsoleLogService.hpp:16` | |
| **A21 — stock app does not publish DemoLogEvent in production** | **Verified** | grep src/: `events::publish` only in EventSystem.hpp implementation + test files | |
| **F19 — aqtinstall pinned to git commit** | **Verified** (was unverified in phase-0) | `Pipfile:13` full commit `076e1659…`; Pipfile.lock records git pin | Closed by 2E. |
| **F1–F8, F10–F18, F20–F26 — build facts** | **Verified** | See 2E claim table — all match code | |
| **C1–C10 — service registration lifecycle** | **Verified** | `main.cpp:62/68/69-70/96/104/113/117`; `ServiceRegistry.cpp:8-23` | |
| **D1–D11 — boundary rules/exemptions/heuristic honesty** | **Verified** | check-architecture-boundaries.sh rules match AGENTS.md numbering; self-documented heuristic | |
| **B1–B7, B9–B14, B16–B20 — persistence claims** | **Verified** | See CLAIMS-CHECKLIST §B + agent re-checks | |
| **K1–K10 — logging claims** | **Verified** | `logging.{h,cpp}`; `main.cpp` | |
| **L1–L7 — lifecycle claims** | **Verified** | AppMainWindow closeEvent; main.cpp paths; EventSystem quit hook | |
| **M1–M10 — README claims** | **Mostly verified** | M6 (end-to-end rename) PARTIALLY — consistent with files, rename path not executed | |
| **N1–N10 — pedagogical claims** | **Verified / intentional-only** | No auto-registration macros; boundary honesty; etc. | |

### The 3 material overstatements/partial failures

1. **E10/H9 — m_models assert overstatement.** Debug assert does not "catch forgotten appends" of new feature models and does not "match constructed feature models" in general — it counts a hardcoded census of two reference types. Correct extension false-positives; forgotten append of new type is silent. (AUD-002)
2. **B15 / "provider must outlive models" as enforced invariant — partial.** Convention-only on default path (sibling QObject children; no destruction-order guarantee); structural only on injected path. Safe today because no model has destructor that touches provider. (AUD-001)
3. **F9 — Windows smoke coverage via build.sh — partial.** Mechanism claim code-true; effective coverage not — scripts likely skip production binary under pinned Ninja generator. PR CI on Windows never launches production binary. (AUD-003)

### Claim summary counts (CLAIMS-CHECKLIST, 137 assessed)

| Status | Count |
|---|---|
| VERIFIED IN CODE | 118 |
| PARTIALLY VERIFIED | 6 |
| NOT VERIFIED / STALE | 1 (F19 — later closed by 2E) |
| INTENTIONAL-ONLY | 12 |
| **Total** | **137** |

---

## Appendix: Audit Limitations

1. **Modern C++ (§7) idiom sweep was thin.** PersistenceResult-vs-`std::expected` and C++20 pin addressed (2B/2G); no dedicated ranges/spans/designated initializers/`consteval` sweep; no deep C++20-vs-C++23 justification beyond PersistenceResult.
2. **Windows path defect inferred, not observed.** AUD-003 is static analysis (profiles + scripts + bundle fallback) without Windows execution. Confidence Medium correct. Verify on Windows before treating as confirmed operational fact.
3. **macOS packaging coverage partial.** Linux AppImage (AUD-111) and Windows Inno (AUD-122) audited; mac-bundle-app.sh received only path-duplication observation (AUD-109), not dedicated audit of bundle signing/notarization/.app layout beyond Contents/MacOS path.
4. **Performance verdict LOW is structural reasoning, not measurement.** 2F's analysis sound (zero production publish sites; small JSON; sync persistence by design); no profiling numbers. Matches AUDIT.md §17's own "optimize only with credible evidence" rule.
5. **Dependency architecture (§9) covered at recipe level only.** Conan layout, lockfile superset, CMake link visibility verified; no deep transitive-dependency analysis (e.g., what xorg/system pulls in on Linux, ABI risks of PUBLIC Qt6::Widgets on STATIC lib for hypothetical consumers). Acceptable for single-app template.
6. **Concurrency (§11) single-threaded by design; no multi-thread misuse tests exist.** All agents verified absence of worker threads and EventSystem GUI-thread guards. Untested rejection path (AUD-101) is the concrete gap. What happens if derived app violates policy was not modeled — only that EventSystem rejects loudly and models do not.
7. **Theme/i18n depth partial.** Theme behavior verified. i18n rename flow (M6) checked for file consistency but **not executed end-to-end**.
8. **AI-ergonomics deeper angles open.** 2D scorecard covers 9 questions well (Q2/Q4/Q7/Q9 fully legible; Q1/Q3/Q5/Q6/Q8 partly). Unanswered: provider-lifetime semantics for agents reading only model headers; what happens when a **provider** is destroyed mid-session (model/widget destruction covered; provider destruction paths not test-locked); "where do I put a new logging category" discoverability from headers alone.
9. **Historical AUDIT-REPORT.md excluded by protocol.** This fresh pass did not open, read, or compare against it. Findings stand on code evidence alone.
10. **ID collision between 2D and 2F (F-2xx range).** Resolved by renumbering to AUD-NNN; agent attribution preserved.

---

## Appendix: Remediation Status (applied after this audit)

Waves 1–7 of the migration plan were implemented in a subsequent pass (parallel tracks + integration). Summary:

| Wave | Status | Notes |
|---|---|---|
| 1 Docs corrections | **Done** | AGENTS.md E10/H9, Model vocabulary, provider lifetime, ServiceRegistry scope, config channels, thread-guard asymmetry |
| 2 m_models assert | **Done** | `registerModel(IModel*)` single append site; census assert removed |
| 3 EventSystem tests + quit re-arm | **Done** | Quit lifecycle (subprocess), wrong-thread, order tests; quit flags re-arm (AUD-123 A); QObject subscribe returns Subscription (AUD-105 A) |
| 4 Windows path helper | **Done (code)** | `scripts/lib/resolve-app-path.sh`; Ninja-first + multi-config fallback; CI hard-fail. **Windows machine verification still required** to upgrade AUD-003 confidence |
| 5 QSettings + LoggingTest + MemoryProvider | **Done** | status() observed; behavioral logging tests; InvalidData parity |
| 6 Composition ergonomics + generator | **Done** | `constructFeatures()`; generate.sh checklist + fidelity (logging, non-placeholder stubs, test-generator greps) |
| 7 Tooling hygiene | **Done** | Dockerfile gcc-15 + CI compiler guard; AppImage 1.9.1 pin + vendored excludelist; pipenv `--deploy`; inno.iss DIST_DIR; QSS list from qrc; autogen gen-expr documented (hardcode kept) |
| Wave 8 applied | **Partial** | ReusableWidget → `examples/widgets/reusable/`; QObject Subscription return done |
| Wave 8 deferred | **Documented** | Multi-target layering; `std::expected` (C++23); pipenv→uv; EventSystem cross-thread; hand-rolled CMakePresets (Conan already generates) |
| AUD-114 | **Deferred** | main.cpp exception-path unregister covered by qScopeGuard + explicit calls; subprocess death-test impractical without restructuring main |

**Post-remediation verification (Linux):** `./scripts/build.sh --test ON --clean ON` → **113/113** (1 intentional in-process skip; quit path covered by subprocess test); production smoke OK; architecture boundaries OK. `./scripts/test-generator.sh` → **PASSED** (115/115 including generated feature tests).

**Still open for the maintainer:**
1. Run `./scripts/build.sh --test ON` on a **Windows + Ninja** host and confirm smoke-test launches (AUD-003).
2. Commit strategy (waves as separate commits recommended; nothing committed in the remediation pass unless requested).
3. Wave 8 deferred items remain revisit-only.

---

*End of AUDIT-REPORT-2. Written to `/home/christian/Programming/qt6-template/AUDIT-REPORT-2.md`. Historical AUDIT-REPORT.md was never opened.*
