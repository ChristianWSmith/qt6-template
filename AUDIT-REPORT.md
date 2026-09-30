# Architectural & Design Audit — Qt 6 Application Template

**Date:** 2026-09-30  
**Scope:** Full architectural review per `AUDIT.md`  
**Method:** Multi-agent reconstruction (architecture cartographers) → domain deep-dives (EventSystem, persistence/error, Qt6 un-workarounds, ownership/modern C++, features/ergonomics, build/CI) → synthesis → quality gate  
**Repository state:** Analysis only. No repository files were modified during the audit. Intermediate agent reports lived under `/tmp/opencode/qt6-audit/` and were cleaned up after this deliverable was written.

---

## A. Executive Summary

### Overall assessment

This template is **technically sound**. It is an opinionated Qt 6 + CMake + Conan application template that deliberately packages:

1. A composition-root feature architecture (Model / Presenter / Widget MV\*)
2. A typed, always-queued event bus for genuinely decoupled cross-component events
3. An explicit typed persistence error channel with a testability seam
4. An `app.env` metadata SSOT with fail-fast across Conan / CMake / scripts
5. A generator + CI-checked convention loop for new features
6. A three-OS build / CI / packaging pipeline with real dev–CI parity via scripts

Across **42 investigated custom mechanisms**, **zero classify as E (actively harmful)** and only **three classify as D (historical/accidental)** — and none of those three is a classic “we worked around Qt X” historical workaround. They are accidental build coupling and documentation overclaims.

**Findings:** 55 total — Critical **0** · Significant **7** · Improvement **24** · Opinion **6** · Keep **14** · Investigate **2** · (plus AI-ergonomics findings folded into the register).  
**Un-workarounds:** 42 — A **10** · B **19** · C **10** · D **3** · E **0**.

### Where the repository fights Qt / C++

**Almost nowhere.** Investigation leads that could have been “fighting Qt” all closed as canonical-mechanism-used-correctly or justified-custom-abstraction:

| Lead | Verdict |
|---|---|
| EventSystem vs signals/slots | Wraps Qt queued connections; unique deltas are real (open typed topics, always-queued contract, RAII free-function subscribe) |
| PersistenceResult vs `std::expected` | C++23 feature; project pins C++20; misuse contract differs even under C++23; error taxonomy load-bearing |
| QObject parenting vs `unique_ptr` | Parent-child is **structurally safer** given Qt child-destruction timing |
| Theme vs QPalette | Uses Qt 6.5+ `QStyleHints::colorScheme()` + canonical `setStyleSheet` |
| `apply_app_metadata` vs Qt | Verified against Qt 6.11 docs: `qt_add_executable` has **no** metadata args |
| cxxopts / fmt vs Qt CLI/strings | Documented intentional Conan dependency demonstrations |
| QSettings geometry, `.moc`, custom gtest main, `configureLogLevel` | All canonical mechanisms |

The strongest criticisms are **contract-strength defects** (docs/comments that overclaim what code guarantees) and **claim-reality gaps** (AGENTS.md/README vs files), not broken architecture.

### Strongest architectural decisions

1. **AGENTS.md as machine-legible contract** that matches code on ownership, events, async, persistence, services, threading — with honest live-vs-demo posture for EventSystem/LogEvent/AppLog. This is the template’s primary AI-legibility asset.
2. **Composition-root Qt parent-child ownership** with construction-order encoding (provider first; model+widget before presenter) — sound, tested, generator-reproduced.
3. **IPersistenceProvider seam + PersistenceResult** — real testability and explicit typed errors under C++20. Taxonomy load-bearing (`NotFound` = first run).
4. **EventSystem as intentional teaching artifact** — packages semantics Qt does not ship as one facility; zero production publishers is stated intent; tests lock the contract.
5. **generate.sh + test-generator.sh** — conventions are mechanically enforced, not aspirational.
6. **app.env SSOT + fail-fast + scripts façade + CI parity** — one-file branding; CI runs the same path as dev.
7. **Async non-goal + synchronous persistence policy** — explicit six-step escalation before workers; matches actual payload scale.

### Largest architectural risks

1. **Contract-strength defects that AI agents will follow** (F-02 destruction-order overclaim; F-03 falsified event-type contract; F-04 wrong test-mirror path; F-S2-01 missing layout step; F-06 tidy claim without enforcement). An agent adding a presenter destructor that dereferences `m_view` would be told construction order protects them — it does not under Qt’s public contract.
2. **Error semantics stop at the model layer** (F-01) — “explicit persistence errors” means “explicit in logs only”; `closeEvent` never sees save results.
3. **Lifecycle teardown is incidental, not designed** (F-08) — static `Subscription` destruction after `QApplication` relies on Qt stale-handle no-op; untested in that order.
4. **Build coupling to static-archive extraction** (F-05) — `main.cpp` in the static lib works by linker rules, not design.
5. **Claim-reality gaps erode trust in docs** — wrong docs are worse than missing docs for a template consumed by agents.
6. **Unenforced architectural boundaries** (F-17) — layering rules are convention-only; agents can violate them with green CI.

### Biggest opportunities for simplification

1. Replace custom `messageHandler` with `qSetMessagePattern` (keep categories + `configureLogLevel`)
2. Adopt `IModel` polymorphically (model list + `closeEvent` loop) **or** remove it
3. Simplify free-function `Subscription` to app-as-context (drop per-subscribe wrapper)
4. Move `main.cpp` to the exe target
5. Align `BUILD_TESTING` defaults; document or remove `UT_NAME`
6. Fix lock-update delete-before-merge hazard
7. Doc-drift sweep (test paths, README tree, build facts, layout step, config channels)

### Most important un-workarounds

- **Keep (B):** EventSystem bus; QVariant erasure; BusRegistry singleton + raw map; Subscription RAII; `qRegisterMetaType`; IPersistenceProvider + PersistenceResult; abort-on-misuse; save-in-closeEvent; dual persistence channels; AppLog persistence demo; MVP convention; presenter layer; cxxopts; fmt; app.env SSOT; scripts façade; Conan lock merge strategy; GLOB_RECURSE
- **Improve (C):** wrapper QObject per subscribe; static Subscription storage pattern; IModel marker; friend-class scaffolding; lock-delete-before-merge hazard; Dockerfile dual Qt; UT_NAME; triple BUILD_TESTING defaults; copy_shared_libs duplication
- **Un-workaround with coherent story (D):** runtime type check + public dispatcher exposure; main-in-lib; clang-tidy claim without enforcement
- **Necessary (A):** load-in-ctor; ConsoleLogService scaffolding; apply_app_metadata; theme loading; ReusableWidget; custom gtest main; install-qt 7z workaround; `.moc` includes; configureLogLevel; logging/EventSystem separation

### Areas that should explicitly remain opinionated

1. EventSystem reserved for genuinely decoupled events only; intra-feature uses Qt signals/slots; diagnostics never on the bus
2. No production LogEvent publisher in the stock app — do not add a fake publisher “to make the path live”
3. Synchronous GUI-thread persistence + no background-worker architecture without the six-step process
4. cxxopts + fmt as Conan dependency demonstrations
5. Explicit service registration (no auto-registration macros)
6. Do not expand unused interface markers (either adopt or remove IModel)
7. String Boundary Policy (`std::string` at bus, `QString` at UI)
8. `custom.qss` empty = Qt default styling
9. app.env SSOT + fail-fast — no silent fallback defaults for `APP_*` metadata

---

## B. Architecture Map

### Major components

```text
Application
    |
    +-- main.cpp                          bootstrap; QApplication; logging; registerAll(); theme
    |
    +-- AppMainWindow                     composition root (sole owner of feature graphs)
    |     |-- FilePersistenceProvider     QObject child; QSaveFile JSON under AppDataLocation
    |     |-- CounterModel / CounterPresenter / CounterWidget
    |     |-- AppLogModel / AppLogPresenter / AppLogWidget
    |     +-- QSettings window geometry/state (chrome, not feature state)
    |
    +-- EventSystem (BusRegistry + EventDispatcher<T> + Subscription)
    |     |-- LogEvent (std::string)      subscribed in prod; never published in prod
    |     +-- test-only bus types
    |
    +-- services::registerAll()           process-level; free-function Subscription storage
    |     +-- ConsoleLogService           EventSystem free-function subscriber
    |
    +-- logging                           qC* categories + custom messageHandler + configureLogLevel
    |
    +-- platform/theme                    QSS from :/styles/; colorScheme-aware
    |
    +-- widgets/reusable                  teaching artifact; not instantiated
    |
    +-- app.env SSOT                      scripts/env.sh -> Conan -> CMake -> compile defs
```

### Module boundaries

| Area | Path | Role |
|---|---|---|
| Core contracts | `src/core/` | `IModel`, `IPersistenceProvider` + `PersistenceResult` |
| Events | `src/events/` | EventSystem + LogEvent |
| Features | `src/features/<name>/{model,presenter,widget}` | MV\* feature slices |
| Services | `src/services/` | Explicit EventSystem registration + demo handler |
| Platform | `src/platform/` | FilePersistenceProvider, theme |
| Logging | `src/logging/` | Categories + handler + filter rules |
| Composition | `src/appmainwindow/`, `src/main.cpp` | Wiring + bootstrap |
| Widgets | `src/widgets/reusable/` | Teaching artifact |

### Ownership relationships

```text
Owner -> Lifetime -> Owned Object

QApplication
    +-- AppMainWindow (parent)
          +-- FilePersistenceProvider (parent)
          +-- CounterModel / CounterWidget / CounterPresenter (parents)
          +-- AppLogModel / AppLogWidget / AppLogPresenter (parents)
          +-- containerWidget -> feature widgets reparented into layout
          +-- QSettings chrome (no QObject ownership)
    +-- EventSystem dispatchers + free-function wrapper QObjects (parented to QCoreApplication)
    +-- BusRegistry Meyers singleton (C++ static; non-owning raw-pointer map)

Presenter
    +-- raw non-owning pointers to model + widget

Model
    +-- required non-owning IPersistenceProvider& (bound in ctor init list)

Free-function Subscription
    +-- caller-owned RAII handle; [[nodiscard]]; reset() disconnects only
```

**Important:** `AppMainWindow.h` currently claims construction order guarantees destruction order. Qt 6 public docs do **not** guarantee destruction order for heap QObject trees (“can be destroyed in any order”). Widgets are reparented into `containerWidget` created *after* presenters — under typical Qt reverse-deletion the widget subtree dies before presenter destructors. Safe today only because presenter dtors are trivial and Qt auto-disconnects. This is finding **F-02**.

### Event / data flow

```text
Producer -> Event/Signal -> Consumer

UI button -> Widget intent signal -> Presenter -> Model
Presenter -> Model state signal -> Presenter -> Widget display

Model <-> IPersistenceProvider (synchronous load in ctor; save in closeEvent)

Diagnostic path (NOT EventSystem):
qC* category -> custom messageHandler (mutex) -> stdout/stderr

Demonstration path (EventSystem, silent in stock app):
events::publish(LogEvent) -> EventDispatcher<LogEvent> (queued) ->
    AppLogPresenter (QObject overload) + ConsoleLogService (free-function)
```

### Threading model

- Single GUI thread for all QObject-derived application objects
- No `QThread` / `QtConcurrent` anywhere in `src/`
- EventSystem delivery always `Qt::QueuedConnection`
- `BusRegistry` mutex guards dispatcher *creation*, not delivery
- Logging message handler uses `QMutex`
- Persistence is synchronous on the GUI thread (intentional non-goal)

### Initialization / shutdown order

```text
init:
    1. QApplication
    2. configure logging categories / message handler
    3. services::registerAll()          # EventSystem subs; requires running QApplication
    4. AppMainWindow
         - FilePersistenceProvider first
         - per feature: model (loads state) + widget, then presenter
         - layout code-built: mainLayout->addWidget(...)
    5. theme load
    6. app.exec()

shutdown:
    1. closeEvent: QSettings geometry/state + model saveState() (results ignored today — F-01)
    2. AppMainWindow destruction: QObject tree via parent-child
    3. presenters/widgets/models/providers die with window
    4. QApplication death reclaims EventSystem wrappers/dispatchers
    5. static Subscription destructors run after QApplication (incidental — F-08)
```

### Persistence / configuration channels

| Channel | What | Mechanism |
|---|---|---|
| `app.env` | Build-time metadata SSOT | scripts/env.sh → Conan → CMake fail-fast → `apply_app_metadata()` compile defs |
| QSettings | Window/UI chrome | `QSettings(ORGANIZATION_NAME, APP_NAME)` in `AppMainWindow` |
| `IPersistenceProvider` | Feature state | JSON files via `FilePersistenceProvider` (QSaveFile atomic replace); test double `MemoryPersistenceProvider` |

### Error propagation

- Provider layer: explicit `PersistenceResult<T>` / `PersistenceResult<void>`
- Model layer: log-and-continue (`qCWarning` + `toString(error)`)
- Composition root: **ignores** save results entirely (F-01)
- `PersistenceResult` misuse: assert in debug, `std::abort` in release (deliberate; residual AI-code risk F-26)
- EventSystem type mismatch: `qCritical(appEvent)` + drop delivery
- Assertions: `Q_ASSERT` in debug paths; generator templates carry NOLINT blocks

### Build / test architecture

- C++20; AUTOUIC/AUTOMOC/AUTORCC; `GLOB_RECURSE CONFIGURE_DEPENDS`
- Targets: `${APP_NAME}_lib` (STATIC, currently includes `main.cpp`) → `${APP_NAME}` exe → `${UT_NAME}` tests
- All linkage PRIVATE; tests re-link Qt6/fmt/GTest and add autogen include for `ui_*.h`
- Tests: single GTest binary; custom `QApplication` main + `QStandardPaths` test mode; `gtest_discover_tests` (POST_BUILD also runs discovery)
- CI mirrors `scripts/build.sh` on Linux/Windows/macOS; generator smoke; bundles only on main
- Packaging entirely outside CMake install rules (exe-only `install()`)

### Extension points

| Point | Status |
|---|---|
| `generate.sh feature/ widget` | Live; CI-checked against reference conventions |
| `custom.qss` | Live override; empty = Qt default |
| `services::registerAll()` | Live explicit registration |
| EventSystem publish/subscribe | Live facility; stock app has subscribers, no publishers |
| `app.env` | Live SSOT |
| `ReusableWidget` | Teaching artifact; not instantiated |

---

## C. Findings

Severity order. Full evidence retained for Significant items; remaining items complete the schema in condensed form. Confidence: High / Medium / Low.

---

### Significant

```text
FINDING: closeEvent ignores persistence save failure results
ID: F-01
Severity: Significant
Confidence: High
Category: Persistence / ErrorHandling
Location: src/appmainwindow/AppMainWindow.cpp:55-56; src/core/IModel.h:22-23
ArchitectureClass: Intentional (log-and-continue) with accidental composition-root blindness

Current Design:
    Model::saveState() returns void; provider failures are logged inside models
    via qCWarning + toString(error). closeEvent calls m_counterModel->saveState()
    and m_appLogModel->saveState() with no observation or reaction; the window
    closes regardless.

Problem:
    The explicit PersistenceResult channel never reaches the composition root.
    Disk-full / permission / durability failures at shutdown are invisible to
    the user and to any future "confirm before close" logic. "Explicit
    persistence errors" currently means "explicit in logs only."

Why It Matters:
    Template consumers extending features inherit silent data loss on close.
    An AI agent reading closeEvent will not see that save results matter — the
    API shape teaches that they do not.

Canonical Qt/C++ Mechanism:
    Propagate results: saveState() -> PersistenceResult<void>; closeEvent
    aggregates, optionally shows a status message; or keep void but add a model
    signal saveFailed(PersistenceError).

Alternatives:
    A) Change IModel::saveState to return PersistenceResult<void>; models forward
       provider result; closeEvent logs + (optional) UI notice.
    B) Keep void; add signal on models; presenter could react.
    C) Keep as-is; document loudly that save failures are log-only.
    D) Abort-on-save-failure — too harsh for a template.

Recommendation:
    A or C. A is the small correct fix aligned with the template's own
    "explicit errors" philosophy; C is acceptable if the log-only policy is
    made an explicit AGENTS.md invariant. Do not leave the gap undocumented.

Migration Complexity: Low
Risk of Change: Low

Evidence:
    - AppMainWindow.cpp:55-56 (unchecked calls)
    - CounterModel.cpp:41-47 (hasError -> qCWarning, void return)
    - AppLogModel.cpp:66-80 (same pattern)
    - IModel.h:22-23 (void loadState/saveState)
    - AGENTS.md Persistence: "Persistence failures are explicit via
      PersistenceResult<T>" — true at provider, false at composition root
```

```text
FINDING: Presenter lifetime comment overclaims Qt destruction-order guarantee
ID: F-02
Severity: Significant
Confidence: High
Category: Ownership
Location: src/appmainwindow/AppMainWindow.h:28-35; AppMainWindow.cpp:34-43
ArchitectureClass: Accidental (contract comment overstates what structure guarantees)

Current Design:
    Presenters hold CounterModel*/CounterWidget* (and AppLog equivalents) as
    non-owning raw pointers, documented as: "model and widget are constructed
    before the presenter and owned via Qt parent-child. The presenter does not
    outlive its dependencies."

Problem:
    The comment presents construction order as a destruction-order guarantee.
    Qt 6 public documentation does not provide one for heap QObjects: "a tree
    can be constructed from them in any order, and later, the objects in the
    tree can be destroyed in any order" (doc.qt.io/qt-6/objecttrees.html).
    Widgets are reparented into containerWidget created AFTER the presenters,
    so under typical Qt reverse-deletion the widget subtree dies BEFORE
    presenter dtors — m_view/m_model dangle during teardown. Safe today only
    because presenter dtors are trivial and Qt auto-disconnects. Test fixtures
    use reverse member destruction and do NOT reproduce production order.

Why It Matters:
    This template is consumed by AI coding agents and humans following its
    documented conventions. The comment teaches a false safety argument. An
    agent adding a user-defined presenter destructor, a teardown-time state
    flush, or any direct call on m_model/m_view during destruction would be
    told construction order protects them — it does not under Qt's public
    contract. Generated features inherit the same comment pattern.

Canonical Qt/C++ Mechanism:
    The real guarantees are: (1) Qt parent-child single ownership; (2)
    signal/slot auto-disconnect on sender or receiver destruction; (3)
    queued-event removal for destroyed receivers. Dtor-time raw-pointer access
    is not covered by any of these.

Alternatives:
    A) Correct the comment to state the actual guarantee. Minimal, honest.
    B) Also reorder construction so typical Qt destruction matches teaching
       intent (still implementation-behavior-dependent unless A is done).
    C) QPointer for m_model/m_view — optional hardening, not a default.

Recommendation:
    Keep raw pointers (the pointer choice is correct — F-35). Apply (A):
    rewrite the contract comment to describe Qt connection auto-disconnect +
    the rule "destructors must not dereference model/widget pointers."
    Optionally apply (B) and mirror dtor-safety expectations into generate.sh
    comments.

Migration Complexity: Low (comment + optional construction reorder)
Risk of Change: Low

Evidence:
    - AppMainWindow.h:28-35 (contract text)
    - AppMainWindow.cpp:34-43 (containerWidget created in body AFTER init-list
      presenters; mainLayout->addWidget reparents widgets)
    - Qt 6 docs: doc.qt.io/qt-6/objecttrees.html
    - CounterPresenter.h / AppLogPresenter.h (no user-defined dtors today)
    - tests/features/CounterTest.cpp member order (opposite of production teardown)
```

```text
FINDING: Event type contract understates actual type requirements
ID: F-03
Severity: Significant
Confidence: High
Category: EventSystem
Location: src/events/system/EventSystem.hpp:47, :96, :137, :200
ArchitectureClass: Accidental

Current Design:
    Contract comment (line 47) and static_assert (line 96) state only:
    "Event types must be copy-constructible." Delivery lambdas additionally
    require default-constructibility (T event{}; lines 137, 200) and
    assignability (out = var.value<T>(); line 63).

Problem:
    The documented compile-time contract is wrong. A copy-constructible but
    not default-constructible event type compiles past the static_assert and
    fails inside the lambda with a confusing error.

Why It Matters:
    AGENTS.md treats EventSystem.hpp as the normative contract. A false
    contract is worse than a strict one. AUDIT.md §14/§15: developers and AI
    agents rely on stated contracts without reading implementation.

Canonical Qt/C++ Mechanism:
    Either (a) strengthen static_assert to require default-constructible +
    copy-assignable, or (b) change delivery to avoid T event{} / value<T>().

Alternatives:
    (a) static_assert(std::is_default_constructible_v<T> &&
        std::is_copy_assignable_v<T>) — smallest change.
    (b) Rewrite delivery to avoid default construction — more code; not justified
        for current event types.
    (c) Document full requirement without strengthening assert — leaves the
        confusing compile error.

Recommendation:
    (a) + contract comment update. A stricter assert rejecting previously
    "compiling" event types that never worked at delivery is a feature.

Migration Complexity: Low
Risk of Change: Low

Evidence:
    - EventSystem.hpp:47, :96, :63, :137/:200
    - LogEvent (std::string member) is default-constructible — stock app unaffected
```

```text
FINDING: AGENTS.md Adding-a-New-Feature omits layout insertion; features can be
         wired and persisted yet remain invisible
ID: F-S2-01
Severity: Significant
Confidence: High
Category: AI-Ergonomics / Docs
Location: AGENTS.md Adding-a-New-Feature vs src/appmainwindow/AppMainWindow.cpp:31-43
ArchitectureClass: Accidental

Current Design:
    AGENTS.md 10-step feature checklist covers generate, wire model/widget/
    presenter, provider dependency, signal connections, closeEvent persistence,
    tests, build. The central layout is code-built in AppMainWindow ctor
    (containerWidget + QHBoxLayout + addWidget). AppMainWindow.ui is
    intentionally minimal.

Problem:
    The checklist never mentions adding the feature widget to the layout.
    An agent that follows AGENTS.md exactly can construct all three objects,
    connect every signal, save state on close — and ship a feature that never
    appears on screen. Silent failure (compiles; tests may pass).

Why It Matters:
    Highest-value AI-ergonomics gap. Layout insertion is required knowledge
    living only in AppMainWindow.cpp, not in the agent contract AGENTS.md
    claims to be. AUDIT.md §15 targets implicit conventions and APIs whose
    correct use requires reading unrelated files.

Canonical Qt/C++ Mechanism:
    Qt layouts: widgets must be added to a layout to be displayed. The
    template deliberately code-builds the central layout — fine, but the
    extension procedure must say so.

Alternatives:
    (A) Docs-only: extend AGENTS.md feature steps with addWidget instruction
        (recommended).
    (B) Move layout into AppMainWindow.ui — rejected; fights the teaching
        artifact (ui intentionally minimal).

Recommendation:
    Add to AGENTS.md Adding a New Feature:
        "Add the feature widget to the central layout in the AppMainWindow
         constructor (mainLayout->addWidget). The central layout is code-built —
         do not add feature widgets to AppMainWindow.ui."

Migration Complexity: Low
Risk of Change: Low

Evidence:
    - AGENTS.md feature checklist (no layout step)
    - AppMainWindow.cpp:31-43 (code-built layout + addWidget)
    - AppMainWindow.ui minimal
    - generate.sh does not emit AppMainWindow wiring (documented manual step)
```

```text
FINDING: Test-mirror layout documentation contradicts flat code layout
ID: F-04
Severity: Significant
Confidence: High
Category: Docs
Location: AGENTS.md test mirror claim; README project structure; actual tests/features/*.cpp flat
ArchitectureClass: Accidental

Current Design:
    Tests live flat: tests/features/CounterTest.cpp, AppLogTest.cpp.
    generate.sh writes flat. AGENTS.md and README claim tests/features/<name>/
    hierarchy mirroring src.

Problem:
    Docs instruct agents/developers to expect per-feature test directories;
    generator and committed tests do the opposite. Agents following docs may
    look for generated tests in the wrong place.

Why It Matters:
    Docs are the AI contract for this template; a wrong path claim is a
    machine-legibility failure.

Alternatives:
    (A) Fix AGENTS.md + README to describe flat layout (recommended).
    (B) Change generator + move tests to hierarchy docs (coordinated Medium).

Recommendation:
    Fix docs to match code unless maintainers actively want hierarchy. Do not
    leave split-brain state. Code + generator + smoke script are consistent
    on flat layout — docs are wrong.

Migration Complexity: Low (docs) / Medium (layout change)
Risk of Change: Low

Evidence:
    - tests/features/CounterTest.cpp, AppLogTest.cpp
    - generate.sh test output path
    - AGENTS.md vs scripts/env.sh TESTS_FEATURES_DIR
```

```text
FINDING: main.cpp compiled into static lib couples tests to static-archive semantics
ID: F-05
Severity: Significant
Confidence: High
Category: Build
Location: CMakeLists.txt (glob includes src/main.cpp in ${APP_NAME}_lib); tests/main.cpp
ArchitectureClass: Accidental (U-36 class D)

Current Design:
    Glob includes src/main.cpp in ${APP_NAME}_lib; exe has only resources;
    apply_app_metadata(lib) not (exe); tests link lib and define their own
    main(); no duplicate main because main.o is unextracted from the static
    archive.

Problem:
    Test linking safety depends on static selective extraction, not on design.
    SHARED lib, --whole-archive, or a future cross-reference from lib code to
    main.cpp symbols breaks UnitTests with duplicate main. Metadata on the exe
    is unapplied and invisible.

Why It Matters:
    Templates are read and modified by AI agents and new developers; a
    non-obvious linker dependency is a trap.

Canonical Qt/C++ Mechanism:
    Entry point compiled into the executable target; libraries hold no main().

Alternatives:
    Keep as-is and document loudly; OBJECT library; explicit lib sources
    excluding main.cpp + main on exe.

Recommendation:
    Move main.cpp to ${APP_NAME} sources; apply_app_metadata(${APP_NAME});
    keep lib for features/core only.

Migration Complexity: Low
Risk of Change: Low

Evidence:
    - CMakeLists.txt glob + target_link_libraries pattern
    - tests/main.cpp own main()
    - tests/CMakeLists.txt links lib
```

```text
FINDING: clang-tidy WarningsAsErrors '*' is not build/CI-enforced despite AGENTS.md claim
ID: F-06
Severity: Significant
Confidence: High
Category: Build / Docs
Location: .clang-tidy; scripts/configure-vscode.sh; .github/workflows/ci.yml; AGENTS.md
ArchitectureClass: Defensive (config) / Accidental (claim-reality gap) — U-38 class D

Current Design:
    Broad check set + WarningsAsErrors '*'; enforcement only through generated
    VSCode clangd; CI has no tidy step; generator templates and main.cpp
    already carry NOLINT blocks.

Problem:
    Documented policy ("clang-tidy warnings are errors") is not a build or CI
    invariant. Editor-local enforcement is invisible to non-VSCode users and
    to CI.

Why It Matters:
    A template's quality contract should be either enforced or honestly scoped.
    Wrong docs are worse than missing docs.

Canonical Qt/C++ Mechanism:
    CI lint job consuming CMAKE_EXPORT_COMPILE_COMMANDS (already ON); CMake
    clang-tidy target; or curated checks.

Alternatives:
    Narrow Checks + drop '*'; editor-only policy with corrected docs; CI tidy
    with baseline file.

Recommendation:
    Immediate: fix AGENTS.md to state editor-side scope. Then either add a CI
    tidy job or curate Checks so '*' is defensible. Do not add more NOLINT to
    generated templates without policy discussion.

Migration Complexity: Low (docs) / Medium (CI job)
Risk of Change: Low

Evidence:
    - .clang-tidy; configure-vscode.sh; ci.yml (no tidy)
    - AGENTS.md key quirks
    - NOLINT in src/main.cpp and generate.sh templates
```

---

### Improvement

| ID | Finding | Location | Confidence | Recommendation |
|---|---|---|---|---|
| F-07 | Free-function wrapper QObject accumulates on subscribe/reset cycles | EventSystem.hpp subscribe/reset | High | Use QCoreApplication as connection context; Subscription holds only Connection (U-02 C). Production impact nil today. |
| F-08 | g_consoleLogSubscription teardown relies on incidental static-destruction order | ServiceRegistry.cpp; main.cpp | High | Add services::unregisterAll() in main; document Qt stale-handle no-op. Do not silently convert service to QObject. |
| F-09 | IModel has no polymorphic consumers; contradicts non-goals (no IPresenter/IWidget markers) | IModel.h; AppMainWindow uses concrete types | High | Prefer polymorphic adoption (model list + closeEvent loop) OR remove + document convention. Never expand. (U-11 C) |
| F-10 | dispatcher\<T\>() public mutable exposure pairs with runtime type-check defense | EventSystem.hpp BusRegistry | High | Pick one coherent story: narrow to detail/friend + debug assert, or document as diagnostic API. Do not remove check while dispatcher remains public. (U-08 D) |
| F-11 | Custom messageHandler largely redundant with qSetMessagePattern | logging.cpp; main.cpp | Medium-High | Replace with qSetMessagePattern + default handler, or document as demonstration like cxxopts. Keep categories + configureLogLevel. (U-05 C) |
| F-12 | LifecycleTest does not exercise feature-state persistence E2E | LifecycleTest.cpp | High | Add CloseSavesFeatureModelState; rename misleading test names. |
| F-13 | AppLogModel loadState does not re-apply MAX_LOG_SIZE | AppLogModel.cpp | Medium | Cap load at MAX_LOG_SIZE after load. |
| F-14 | Persistence contract wording overstated ("durable"); error classification asymmetric between load/save; parse double-log | IModel.h; models | High/Medium | Reword to "atomic replace"; single log site; document NotFound = first-run as intentional. |
| F-15 | Test fixture destruction order diverges from production teardown order | tests/features/* | High | Add teardown-order discipline test (destroy view/model before presenter). |
| F-16 | Persistent-configuration channel is undocumented (QSettings vs provider vs app.env) | AppMainWindow QSettings vs AGENTS.md | High | AGENTS.md "Configuration channels" paragraph. |
| F-17 | Architecture-boundary rules unenforced by tests (widget no model includes; diagnostics never on bus) | tests/*; AGENTS.md | High | Optional include-pattern checks in CI/tests — highest-leverage gap for agent-extended templates. |
| F-18 | Documentation drift — README structure tree; AGENTS.md build facts (POST_BUILD discovery, UT_NAME, install/export, Qt6::Test, ctest outside pipenv, fmt lock example) | README; AGENTS.md | High | Docs-only sweep. |
| F-19 | BUILD_TESTING has three defaults; UT_NAME undocumented indirection | build.sh / conanfile / CTest | High | Align defaults (recommend ON from façade); document or remove UT_NAME. (U-40/41 C) |
| F-20 | conan-lock-update.sh deletes lock before merge and leaks temp locks | scripts/conan-lock-update.sh | High | Write temp, mv on success, cleanup. (U-37 hazard) |
| F-21 | Persistence keys unsanitized at provider boundary | FilePersistenceProvider.cpp | Medium | Document key constraints; optional validation. |
| F-22 | Tests couple to ${APP_NAME}_lib_autogen/include for ui_*.h | tests/CMakeLists.txt | Medium | Comment rationale; accept for template. |
| F-23 | aqtinstall pinned to git master | Pipfile | High | Pin to a release; re-verify Windows 7z workaround. |
| F-24 | Dockerfile parallel toolchain contradicts script-path Qt and AGENTS.md Vulkan claim | Dockerfile vs env.sh | High | Align container with script path (toolchain-only; drop unused system Qt + Vulkan). (U-39 C) |
| F-25 | AUTOMOC family and CXX_STANDARD set both explicitly and via qt_standard_project_setup | CMakeLists.txt | Medium | Pick one story (helper or explicit). |
| F-26 | PersistenceResult misuse aborts in Release; untestable contract for AI-generated code | IPersistenceProvider.h | High | Generator comments + optional value() helpers. Residual risk in generated feature code. |
| F-S2-02 | Qt Designer auto-connect (on_*_clicked) implicit convention undocumented | widgets + generate.sh comments | High | One AGENTS.md bullet: slot name must match .ui object name. |
| F-S2-03 | Persistence keys embed APP_ID compile definition without Persistence-section explanation | models + apply_app_metadata | Medium | One clause: APP_ID is a compile def from app.env. |
| F-S2-04 | Model saveState swallows PersistenceResult while AGENTS.md says failures are "explicit" | models vs AGENTS.md | High | One clause: provider layer owns explicit errors; model layer is log-and-continue. |
| F-S2-06 | registerAll() ordering only visible in main.cpp | main.cpp:93 | Medium | One clause: services are process-level; called after QApplication; no AppMainWindow wiring. |

### Opinion

| ID | Finding | Location | Notes |
|---|---|---|---|
| F-27 | BusRegistry Meyers singleton is a justified convenience | EventSystem.hpp | Keep; reconsider only if multiple isolated buses required. Do not add injection seam speculatively. |
| F-28 | Publish-path by-value + move chain is unnecessary | EventSystem.hpp | Bundle with other EventSystem touches; const T& simpler. |
| F-29 | Redundant static_cast to IPersistenceProvider& at composition root | AppMainWindow.cpp | Trivial cleanup; cargo-cult risk for agents copying the pattern. |
| F-30 | Production test-observability counter (receivedCount) is acceptable scaffolding | ConsoleLogService.hpp | Optional: test-only accessor. |
| F-31 | CI does not exercise packaging on pull requests | ci.yml | Main-only is defensible; optional cheap PR bundle smoke. |
| F-32 | AppLog clear() and trimming interact with closeEvent-only save | AppLogModel.cpp | Document policy; do not couple UI action to I/O now. |

### Keep

| ID | Finding | Why justified |
|---|---|---|
| F-33 | EventSystem architecture justified | Packages open typed topics + always-queued + RAII free-function subscribe; wraps Qt delivery; test-locked contract. (U-01/03/04/06/07 B) |
| F-34 | EventSystem/LogEvent demonstration posture intentional | Zero production publishers is documented pedagogy, not dead infrastructure. Tests exercise full publish path. Do not add fake publisher. |
| F-35 | Composition-root Qt parent-child ownership sound | unique_ptr alternatives would be *less* safe (member dtors run before ~QObject children). Keep provider QObject parenting + construction-order encoding. |
| F-36 | Models' IPersistenceProvider& required-reference contract sound | Encodes mandatory dependency at compile time; enables MemoryPersistenceProvider seam. |
| F-37 | PersistenceResult as C++20 expected-substitute justified | std::expected is C++23; different misuse contract even under C++23; taxonomy load-bearing (NotFound = first-run). (U-09 B) |
| F-38 | Synchronous GUI-thread persistence justified | Matches async non-goal + payload scale; preserves local reasoning. |
| F-39 | FilePersistenceProvider QSaveFile + dual persistence channels intentional | QSaveFile atomic replace correct; QSettings-for-chrome / provider-for-feature-state split is intentional. |
| F-40 | MemoryPersistenceProvider seam thin, correct, useful | Enables feature tests with zero filesystem; strongest justification for provider abstraction. |
| F-41 | Feature MVP convention + generate.sh + teaching references sound | Counter + AppLog complementary; presenters earn keep at AppLog complexity and as uniform teaching policy; CI enforces conventions. |
| F-42 | Extension-point documentation honest about live vs demonstration | Strongest AI-legibility asset; preserve language when editing. |
| F-43 | cxxopts + fmt are documented template opinions | AGENTS.md explicitly forbids removing cxxopts solely because Qt has QCommandLineParser. Intentional demos. (U-22/23 B) |
| F-44 | app.env SSOT + fail-fast + scripts facade + CI parity justified | One-file branding; no silent MyApp binaries; CI parity is real. (U-24/25 B) |
| F-45 | Build/test infrastructure keeps: PRIVATE linkage, single UnitTests, custom gtest main, GLOB, theme APIs, apply_app_metadata, .moc includes, 7z workaround, ReusableWidget | Verified canonical or intentional. apply_app_metadata: Qt has no equivalent. Custom gtest main: gtest_main cannot construct QApplication + test-mode QStandardPaths. |
| F-46 | BusRegistry non-owning map + Meyers singleton correct Qt interop | QObject parent-child owns dispatchers; registry non-owning (avoids double-delete); mutex creation-only. |

### Investigate

```text
FINDING: Cross-subscriber order claim is Qt-derived, not test-locked
ID: F-47
Severity: Investigate
Confidence: Medium
Location: AGENTS.md EventSystem ordering claims; tests/events/EventsTest.cpp
Problem: AGENTS.md states cross-subscriber order follows connection creation
    order. Mechanism is Qt signals/slots (verified) but no test locks
    multi-subscriber delivery order. Is this contract or implementation note?
Evidence that would resolve: maintainer policy decision. If contract: add
    EventsTest asserting multi-subscriber order. If note: qualify docs:
    "Qt-semantics-derived; not separately test-locked."
```

```text
FINDING: Conan profiles pin compilers that may not match CI runners
ID: F-48
Severity: Investigate
Confidence: Medium
Location: conan/profiles/*; .github/workflows/ci.yml
Problem: Profiles pin gcc 15 / apple-clang 15 / msvc 194. Whether resolution
    uses profile settings while compilation uses runner toolchains is
    unverified. xorg/system in conan.lock not declared in conanfile.py
    (transitive or residue — unverified).
Evidence that would resolve: compare profile settings to actual CI runner
    toolchains; determine lock residue; clarify windows profile msbuild vs
    Ninja generator.
```

---

### Additional AI-ergonomics findings (S2)

| ID | Severity | Finding |
|---|---|---|
| F-S2-02 | Improvement | `on_*_clicked` auto-connect undocumented in AGENTS.md |
| F-S2-03 | Improvement | APP_ID compile-definition role under-documented in Persistence section |
| F-S2-04 | Improvement | Model saveState swallows PersistenceResult while AGENTS.md says failures are explicit |
| F-S2-05 | Opinion | Free-function subscribe wrapper leak if no QApplication (policy-covered; documented boundary) |
| F-S2-06 | Improvement | registerAll() call-site ordering only visible in main.cpp |
| F-S2-07 | Keep | AGENTS.md is sufficient as agent contract with docs-only fixes — no architecture rewrite needed |

### AI-agent discoverability scorecard (AUDIT.md §15)

| Question | Discoverability | Notes |
|---|---|---|
| How do I add a new service? | easy | AGENTS.md Service Registration matches code |
| How do I subscribe to this event? | easy | Dual overload + Subscription documented in AGENTS.md + header |
| Who owns this object? | easy (features) / needs file hunt (provider, EventSystem wrappers) | Ownership section could add provider + Subscription rows |
| How do I perform asynchronous work? | easy | Explicit non-goal + six-step process |
| How do I add persistent configuration? | ambiguous | Three channels; QSettings chrome undocumented in AGENTS.md |
| How do I expose state to the UI? | needs file hunt / misleading | Layout insertion missing from checklist (F-S2-01) — silent invisible feature |
| How do I clean this object up? | easy | Unusually well documented at every pointer/handle site |
| Can this function be called from another thread? | easy | Negative answer + mutex-scope clarification complete |
| What happens when this object is destroyed? | easy | Composition-root contract + header comments; false destruction-order claim is the exception (F-02) |

### Doc-vs-code discrepancies (quality gate)

| # | Doc claim | Code reality | Wrong side | Severity |
|---|---|---|---|---|
| D1 | Test mirror is `tests/features/<name>/` | Flat `tests/features/*.cpp` | Docs | Significant F-04 |
| D2 | "clang-tidy warnings are errors" | Editor-only; no CI tidy | Docs overclaim | Significant F-06 |
| D3 | Construction order = destruction-order guarantee | Qt: "destroyed in any order" | Code comment | Significant F-02 |
| D4 | saveState "durable before returning" | QSaveFile atomic replace; no power-loss guarantee | Code comment | Improvement F-14 |
| D5 | Event types must be copy-constructible | Delivery also needs default-constructible + assignable | Code comment | Significant F-03 |
| D6 | README project tree shows flat src/ | Modular feature-first layout | Docs stale | Improvement F-18 |
| D7 | README lock example fmt range stale | conanfile requires newer fmt | Docs stale | Improvement F-18 |
| D8 | "All scripts use pipenv run" | ctest in build.sh runs directly | Docs imprecise | Improvement F-18 |
| D9 | "Shutdown is synchronous and deterministic" | Subscription static teardown incidental | Docs overclaim nuance | Improvement F-08 |
| D10 | Deps list incomplete (Qt6::Test, ninja, lock extras) | tests need Qt6 Test; ninja build_requires | Docs incomplete | Improvement F-18 |
| D11 | "No Vulkan" | True of CMake/Conan graph; Dockerfile installs libvulkan* | Dockerfile contradicts | Improvement F-24 |
| D12 | AGENTS.md omits main-in-lib | main.cpp in static lib | Docs omit | Significant F-05 |
| D13–D16 | install/export, POST_BUILD discovery, QSettings channel, non-APP_* fallbacks | Present in code; absent or easy-to-misread in docs | Docs omit | Improvement F-18/F-16 |
| D17–D18 | EventSystem production-status + behavioral contract table | Accurate; verified against code | Docs accurate | Keep F-33/F-34 |

---

## D. Un-Workaround Registry

Classification: **A** Necessary · **B** Justified abstraction · **C** Redundant · **D** Historical/accidental · **E** Actively harmful

### Summary table

| ID | Mechanism | Class | One-line |
|---|---|---|---|
| U-01 | EventSystem typed event bus | B | Open typed topics + always-queued + RAII free-function subscribe |
| U-02 | Free-function subscribe wrapper QObject | C | App-as-context achieves same semantics without per-subscribe allocation |
| U-03 | QVariant type erasure on dispatcher signal | B | Cost of open typed topics without a moc hub |
| U-04 | BusRegistry Meyers singleton + raw-pointer map | B | Correct Qt interop; process-wide by contract |
| U-05 | g_consoleLogSubscription file-scope static | C | Shutdown-order nuance; document + optional unregisterAll |
| U-06 | Subscription RAII type | B | Qt does not ship an RAII handle for free-function connections |
| U-07 | qRegisterMetaType on first dispatcher use | B | Explicit registration conventional and harmless |
| U-08 | Runtime QVariant type check + public dispatcher exposure | D | Coherent only with public dispatcher; pick one story |
| U-09 | IPersistenceProvider + PersistenceResult seam | B | Typed errors + testability; C++20 constraint active |
| U-10 | PersistenceResult abort-on-misuse accessors | B | Fail-fast contract; residual risk in AI-generated code |
| U-11 | IModel marker interface | C | No polymorphic consumers; contradicts non-goals |
| U-12 | load-in-ctor persistence convention | A | Necessary for this template's design |
| U-13 | save-in-closeEvent only | B | Justified simplification; crash-loss is known limitation |
| U-14 | Dual persistence channels (QSettings + JSON) | B | Qt-native split preserving provider test seam |
| U-15 | AppLogModel persistence of diagnostic logs | B | Teaching demo with documented disclaimers |
| U-16 | ConsoleLogService EventSystem subscriber | A | Teaching scaffolding; stock app never publishes LogEvent |
| U-17 | MVP feature convention + generate.sh | B | Template structure; not fighting Qt |
| U-18 | Presenter layer (uniform across features) | B | Teaching uniformity + AppLog layer hygiene |
| U-19 | Friend-class test declarations | C | Currently unused by tests; harmless scaffolding |
| U-20 | ConsoleLogService::receivedCount test probe | B | Minimal observability for registration test |
| U-21 | common.h friend-forward-decl / shared types | C | Tiny ceremony |
| U-22 | cxxopts CLI dependency | B | Documented intentional Conan demonstration |
| U-23 | fmt formatting dependency | B | Documented intentional Conan demonstration |
| U-24 | app.env SSOT + env.sh + Conan/CMake fail-fast | B | Centralization; fail-fast prevents silent misconfiguration |
| U-25 | Developer script facade | B | DX encapsulation; CI parity is real |
| U-26 | apply_app_metadata compile definitions | A | Verified: Qt has no equivalent facility |
| U-27 | Theme loading (QSS + colorScheme) | A | Canonical mechanism used correctly (Qt 6.5+) |
| U-28 | ReusableWidget teaching artifact | A | Intentional; disposition explicit in AGENTS.md |
| U-29 | Conan lock merge strategy | B | Orchestration of native Conan lock create/merge |
| U-30 | Custom gtest main (QApplication bootstrap) | A | gtest_main cannot construct QApplication + test-mode QStandardPaths |
| U-31 | install-qt.sh Windows 7z workaround | A | Genuine third-party bug mitigation |
| U-32 | CMake GLOB_RECURSE + CONFIGURE_DEPENDS | B | Justified for generator-driven template; documented fallback |
| U-33 | .moc include in test .cpp files | A | Canonical Qt/AUTOMOC requirement |
| U-34 | configureLogLevel string→filter-rules mapping | A | Canonical QLoggingCategory::setFilterRules |
| U-35 | EventSystem LogEvent vs diagnostic logging separation | A | Intentional correct separation |
| U-36 | main.cpp compiled into static lib | D | Couples tests to static-archive extraction semantics |
| U-37 | Single merged conan.lock (delete-before-merge hazard) | C | Merge strategy B; script hazard accidental |
| U-38 | clang-tidy WarningsAsErrors without build/CI enforcement | D | Claim-reality gap |
| U-39 | Dockerfile system-Qt parallel to aqt script path | C | Accidental accumulation |
| U-40 | UT_NAME indirection for single test binary | C | No evidence of multi-binary layout |
| U-41 | Triple BUILD_TESTING defaults | C | Accidental: three defaults by entry point |
| U-42 | conanfile copy_shared_libs vs packaging scripts | C | Duplicate layout knowledge; acceptable local DX |

**Class distribution:** A 10 · B 19 · C 10 · D 3 · E 0 · Total 42

### Records (high-value entries)

```text
UNWORKAROUND: Typed event bus over Qt queued connections
ID: U-01
Current workaround:
    Process-wide type-keyed pub/sub bus (BusRegistry + EventDispatcher<T>
    emitting QVariant) with QObject-receiver and free-function subscribe
    overloads; delivery hardcoded Qt::QueuedConnection.
Original problem it appears to solve:
    Cross-component decoupled events where publishers need not hold subscriber
    references; free-function handlers without a QObject context; always-
    asynchronous delivery.
Does that problem still exist?: Yes — Qt signals require a sender QObject at
    connect time; there is no Qt-native free-function subscribe with RAII
    lifetime; there is no Qt-native "always queued" policy knob.
Canonical modern solution:
    Qt signals/slots with Qt::QueuedConnection on a shared QObject bus;
    QEvent/postEvent; QObject::connect with context object for lifetime safety.
Why the canonical solution is or isn't suitable:
    Qt signals cover the QObject-receiver case natively. Qt signals do NOT
    cover: (1) free-function handlers, (2) publisher-side decoupling via
    type-keyed registry, (3) hard-coded always-queued delivery. These are
    meaningful additional semantics. Everything else is Qt behavior reused.
Classification: B
Recommendation:
    Keep the EventSystem. Do not migrate intra-feature flows onto it. When a
    real production cross-component event appears, publish from a semantically
    honest site and document it in AGENTS.md.
Evidence:
    EventSystem.hpp; A2 comparison table; zero production publish call sites;
    AGENTS.md EventSystem sections.
```

```text
UNWORKAROUND: Per-free-function-subscribe wrapper QObject
ID: U-02
Current workaround:
    Each free-function subscribe allocates new QObject(QCoreApplication::
    instance()) as connection context. Subscription::reset() disconnects but
    does NOT delete the wrapper; wrappers accumulate until process teardown.
Original problem it appears to solve:
    Free-function handlers need a QObject context for thread affinity and
    lifetime binding.
Does that problem still exist?: Yes — free functions have no QObject receiver.
Canonical modern solution:
    Use QCoreApplication::instance() itself as the connection context; return
    Subscription holding only QMetaObject::Connection. QObject::disconnect is
    per-connection regardless of shared context.
Why the canonical solution is or isn't suitable:
    Suitable. Same delivery/teardown semantics under GUI-thread policy. Removes
    allocation + accumulation footgun. App-as-context pins delivery to the app
    thread — arguably more correct under documented GUI-thread policy.
Classification: C
Recommendation:
    Simplify: drop per-subscribe wrapper; parent free-function connections to
    QCoreApplication; keep Subscription as RAII over Connection only. Document
    that free-function handlers execute on the application thread.
Evidence:
    EventSystem.hpp subscribe/reset; EventsTest FreeFunction suite; Qt 6
    signalsandslots context-object semantics.
```

```text
UNWORKAROUND: Custom typed persistence result under C++20
ID: U-09
Current workaround:
    PersistenceResult<T> (std::variant<T,PersistenceError>) and
    PersistenceResult<void> (std::optional<PersistenceError>); misuse = assert
    debug / abort release; no exceptions. Errors: NotFound | IoError |
    InvalidData | DurabilityFailure + toString().
Original problem it appears to solve:
    Explicit load/save error channel distinct from QSettings silent-default
    semantics; testability seam via injectable provider.
Does that problem still exist?: Yes — QSettings cannot express the taxonomy;
    optional collapses failures; std::expected is C++23 (project pins C++20)
    and has a different misuse contract even under C++23.
Canonical modern solution:
    std::expected<T,E> (C++23); QSettings for config; optional for optional
    values.
Why the canonical solution is or isn't suitable:
    std::expected unavailable under pinned C++20. optional/QSettings lose the
    error taxonomy. PersistenceResult encodes a different error philosophy
    (fail-fast misuse, no exceptions) coherent with the template's policy.
Classification: B
Recommendation:
    Keep. Revisit only at deliberate C++23 adoption — and even then evaluate
    keeping PersistenceResult for its misuse contract.
Evidence:
    IPersistenceProvider.h; CMakeLists C++20; Conan profiles; CounterModel
    treats NotFound as normal first-run.
```

```text
UNWORKAROUND: IModel load/save interface
ID: U-11
Current workaround:
    IModel marker inherited by CounterModel and AppLogModel; loadState/saveState
    convention documented in AGENTS.md feature checklist.
Original problem it appears to solve:
    Teaching contract for "models own application state and handle persistence."
Does that problem still exist?: Partially — the teaching contract is useful;
    the polymorphic interface is unused. AGENTS.md Non-Goals reject IPresenter/
    IWidget markers, making IModel inconsistent.
Canonical modern solution:
    (A) Remove IModel; document load/save convention in AGENTS.md + generator.
    (B) Adopt polymorphically — AppMainWindow holds QList<IModel*>; closeEvent
        loops saveState().
Why the canonical solution is or isn't suitable:
    B converts the marker into a functioning abstraction and removes composition-
    root boilerplate as features scale. A is acceptable if maintainers want
    minimal types. Expanding IModel would be actively harmful.
Classification: C
Recommendation:
    Prefer B; fallback A. Never expand. If kept for teaching, header comment
    that polymorphic use is optional — convention contract, not dispatch point.
Evidence:
    IModel.h; AppMainWindow calls concrete model types; zero polymorphic
    consumers in src/ or tests/.
```

```text
UNWORKAROUND: Runtime QVariant type check + public dispatcher exposure
ID: U-08
Current workaround:
    checkedEventValue compares meta-type vs qMetaTypeId<T>(); mismatch logs
    critical + drops. BusRegistry::dispatcher<T>() is public, returning
    EventDispatcher<T>& (mutable ref to app-owned QObject). Qt signals are
    public — any caller can emit eventPublished with a foreign QVariant.
Original problem it appears to solve:
    Guard against wrong-typed QVariant delivery. Typed public API makes mismatch
    unreachable in normal use; reachable abuse path is emitting the public
    dispatcher signal directly.
Does that problem still exist?: Only via public dispatcher exposure path.
Canonical modern solution:
    Make dispatcher<T>() private/detail/friend and demote runtime check to
    debug-only assert; OR keep both public + check. Both coherent; current code
    does half of each.
Classification: D
Recommendation:
    Choose one coherent story. Recommended: narrow dispatcher<T> to detail,
    demote check to debug assert. Alternative: document dispatcher<T>() as
    diagnostic/test API. Do not remove check while dispatcher remains public
    and mutable-reference-returning.
Evidence:
    EventSystem.hpp BusRegistry::dispatcher; EventsTest; AGENTS.md contract
    table.
```

```text
UNWORKAROUND: main.cpp compiled into static lib
ID: U-36
Current workaround:
    Glob pulls src/main.cpp into ${APP_NAME}_lib; executable has no sources;
    apply_app_metadata applied only to lib + tests.
Original problem it appears to solve:
    Reuse compile definitions for main() without a separate apply step on the
    exe; keep executable as resource/translation carrier.
Does that problem still exist?: Yes — metadata must reach main() somehow.
Canonical modern solution:
    qt_add_executable(${APP_NAME} src/main.cpp ${RESOURCES});
    apply_app_metadata(${APP_NAME}); lib holds only feature/core sources.
Why the canonical solution is or isn't suitable:
    Suitable. Removes reliance on static-archive selective extraction for tests;
    makes exe a real TU host; metadata application visible on the target that
    needs it.
Classification: D
Recommendation:
    Move main.cpp to ${APP_NAME}; apply_app_metadata on the exe. Low risk.
Evidence:
    CMakeLists.txt glob + targets; tests/main.cpp own main(); static extraction
    semantics.
```

```text
UNWORKAROUND: Third-party CLI parser instead of Qt's QCommandLineParser
ID: U-22
Current workaround:
    cxxopts used for CLI parsing in main.cpp; Conan dependency with version
    range cxxopts/[>=3.3.1 <4].
Original problem it appears to solve:
    AGENTS.md documents cxxopts as an intentional third-party Conan dependency
    alongside fmt (dependency-management demonstration). Prefer QCommandLineParser
    only if Qt-native CLI is a deliberate project choice; do not remove cxxopts
    solely because Qt has an equivalent.
Does that problem still exist?: N/A — the "problem" is template pedagogy, not
    a technical limitation.
Canonical modern solution:
    QCommandLineParser (Qt-native); functionally equivalent for this use.
Why the canonical solution is or isn't suitable:
    Functionally equivalent but not an upgrade given template intent. AGENTS.md
    explicitly forbids removing cxxopts solely because Qt has an equivalent.
Classification: B
Recommendation:
    Keep. Consumer apps may migrate deliberately — latitude already granted.
    Optionally note how to drop it when graduating from template to product.
Evidence:
    AGENTS.md Dependencies section; conanfile.py; main.cpp CLI parsing.
```

```text
UNWORKAROUND: apply_app_metadata compile definitions
ID: U-26
Current workaround:
    CMake function applying APP_NAME/DESCRIPTION/VERSION/ORGANIZATION/APP_ID
    as PRIVATE target_compile_definitions.
Original problem it appears to solve:
    Centralize app metadata compile defs across lib/exe/tests from app.env SSOT.
Does that problem still exist?: Yes for consumers of metadata macros. Qt does
    not provide an equivalent facility.
Canonical modern solution:
    target_compile_definitions (what the function already calls); Qt
    qt_add_executable metadata args (none exist — verified against Qt 6.11 docs).
Why the canonical solution is or isn't suitable:
    Qt 6 qt_add_executable takes no VERSION/AUTHOR/DESCRIPTION metadata args.
    The function is a thin, correct centralization — not a workaround for a
    Qt limitation that no longer exists.
Classification: A
Recommendation:
    Keep. Document that Qt has no equivalent so future agents do not "fix" it.
Evidence:
    CMakeLists.txt apply_app_metadata; Qt 6.11 qt_add_executable docs (verified
    during B3 audit via web).
```

```text
UNWORKAROUND: Custom gtest main (QApplication bootstrap)
ID: U-30
Current workaround:
    tests/main.cpp constructs QApplication, sets QStandardPaths test mode, then
    RUN_ALL_TESTS(); not gtest_main.
Original problem it appears to solve:
    Qt widget tests require a QApplication before test bodies; QStandardPaths
    test mode isolates persistence files from real user data.
Does that problem still exist?: Yes — gtest_main cannot construct QApplication
    + set test-mode paths before test discovery/execution.
Canonical modern solution:
    Custom main (implemented). Qt Test has QTest::qExec but this template uses
    GoogleTest deliberately (documented).
Why the canonical solution is or isn't suitable:
    Necessary. Custom main is the standard Qt+GTest pattern when using GoogleTest.
Classification: A
Recommendation:
    Keep. Not an un-workaround candidate.
Evidence:
    tests/main.cpp; AGENTS.md Testing section.
```

```text
UNWORKAROUND: (non-candidate) LogEvent bus vs qC* diagnostic logging
ID: U-35
Current workaround:
    None — this record closes the investigation lead. Two mechanisms coexist:
    diagnostic logging (qC* + handler) and LogEvent on the bus (subscribed,
    never published in stock app).
Original problem it appears to solve:
    Question: is routing diagnostic logs through EventSystem a missing
    capability, or is the separation intentional?
Does that problem still exist?: The separation is intentional and correct.
    Bus delivery is queued/async; diagnostic logs need synchronous sink-accurate
    output.
Canonical modern solution:
    Diagnostic: qC* + categories + message handler (implemented).
    Domain events: EventSystem publish from semantically honest sites (policy
    documented; no production LogEvent publisher yet — by design).
Classification: A
Recommendation:
    Keep the separation. Do not route diagnostic logs onto the bus.
Evidence:
    AGENTS.md EventSystem production status + String Boundary Policy + Logging
    Categories; logging.cpp (no bus).
```

*(Remaining U-records U-03/04/05/06/07/10/12–34/37–42 are fully specified in the audit working register; classifications and recommendations match the summary table above.)*

---

## E. Architecture Debt Register

| # | Debt item | Description | Evidence | Severity | Linked findings |
|---|---|---|---|---|---|
| AD-01 | Documentation overclaims what code guarantees | Destruction-order comment; "durable" wording; event-type contract; tidy enforcement claim; test-mirror path | B1/B2/B4/B5/B6 | Significant (contract-strength) | F-02, F-03, F-04, F-06, F-14 |
| AD-02 | Error semantics stop at the model layer | PersistenceResult explicit at provider; models log-and-continue; closeEvent ignores results; save errors flattened | B2 | Significant | F-01, F-14 |
| AD-03 | Lifecycle teardown incidental, not designed | Static Subscription dtor after QApplication; wrapper accumulation; no unregisterAll | B1/B4 | Improvement | F-07, F-08 |
| AD-04 | Unused polymorphism / non-goal contradiction | IModel inherited but never consumed polymorphically; AGENTS.md rejects other markers | B2/B3 | Improvement | F-09 |
| AD-05 | API surface exposes internals without documenting contract | Public dispatcher\<T\>() mutable ref; runtime check defends abuse path | B1/B4 | Improvement | F-10 |
| AD-06 | Build coupling to static-archive extraction | main-in-lib; tests work by linker rules not design | B6 | Significant | F-05 |
| AD-07 | Quality-gate claim-reality gap | clang-tidy WarningsAsErrors claimed as errors; editor-only | B6 | Significant | F-06 |
| AD-08 | Implicit conventions without enforcement | Layering rules convention-only; config channels undocumented; auto-connect undocumented | B5/S2 | Improvement | F-04, F-16, F-17, F-S2-01/02 |
| AD-09 | Dual Qt toolchain paths | Dockerfile system Qt + Vulkan vs script-path aqt Qt | B6 | Improvement | F-24 |
| AD-10 | Configuration-knob default divergence | BUILD_TESTING ×3; UT_NAME undocumented; AUTOMOC/CXX standard dual-set; ctest outside pipenv | B6 | Improvement | F-19, F-25 |
| AD-11 | Reproducibility footguns | aqtinstall git master; lock delete-before-merge; profile/runner alignment unverified | B6 | Improvement / Investigate | F-20, F-23, F-48 |
| AD-12 | Test-production lifetime divergence | Fixtures destroy presenter first; production teardown Qt-layout-dependent; no dtor-discipline test | B4 | Improvement | F-15, F-02 |
| AD-13 | Defensive code whose justification depends on undocumented API boundary | Runtime type check + public dispatcher half-state | B1 | Improvement | F-10 |
| AD-14 | Latent misuse surfaces in generated-code target | PersistenceResult abort on misuse; unsanitized keys; generate.sh stubs | B2 | Improvement | F-21, F-26 |

**Summary:** No debt item requires rewriting the architecture. AD-01 (contract overclaims) and AD-02 (error semantics stop at model) are highest-leverage — both documentation/API-shape fixes. AD-06 and AD-07 are claim-reality gaps that undermine trust. **The ownership model itself is not debt** — it is sound; the debt is that documentation overclaims what it guarantees.

---

## F. Keep List

Architecture that should **NOT** be changed.

1. **EventSystem as a typed queued pub/sub bus** — BusRegistry + EventDispatcher\<T\>, always-queued delivery, two subscribe overloads, RAII Subscription ([[nodiscard]], move-only, reset-without-loop), qRegisterMetaType on first use, QVariant type erasure. Packages semantics Qt signals do not offer as one facility. Removing it would delete the only live examples of both subscribe overloads. Do not migrate intra-feature flows onto it. Do not replace with Qt hub signals (loses open-type-set), pure C++ bus (reimplements Qt lifetime badly), or QEvent (wrong shape for pub/sub). *(F-33)*

2. **EventSystem/LogEvent demonstration posture** — Two production subscribers, zero production publishers is documented pedagogy, not dead infrastructure. Mitigation for silent-path misreading risk is a one-line comment at AppLogPresenter’s subscription site, not deletion. Do not route diagnostic logs onto the bus. *(F-34)*

3. **Composition-root Qt parent-child ownership** — AppMainWindow sole composition root; all feature objects parented; provider constructed first; model+widget before presenter per feature. QObject parenting of FilePersistenceProvider is load-bearing and structurally safer than unique_ptr given Qt child-destruction timing. Do not “improve” toward unique_ptr graphs. *(F-35)*

4. **Models’ IPersistenceProvider& required-reference contract** — Required non-nullable reference bound in member init list. Encodes mandatory dependency at compile time; enables MemoryPersistenceProvider seam. *(F-36)*

5. **PersistenceResult\<T\> as C++20 expected-substitute** — Keep custom type with variant/optional storage, hasValue/hasError, abort-on-misuse accessors. Not historical: std::expected is C++23; misuse contract differs; taxonomy load-bearing. Do not import an expected polyfill. *(F-37)*

6. **Synchronous GUI-thread persistence** — load-in-ctor / save-in-closeEvent; no QThread/QtConcurrent without the six-step AGENTS.md escalation process and workload evidence. *(F-38)*

7. **FilePersistenceProvider QSaveFile + dual persistence channels** — QSaveFile atomic writes; QSettings-for-chrome / provider-for-feature-state split. AppLog persistence is a justified teaching demo — do not “fix” by removing persistence. Crash-before-closeEvent loss is a known limitation to document, not a redesign trigger. *(F-39)*

8. **MemoryPersistenceProvider seam** — Thin always-success in-memory double. Primary reason IPersistenceProvider exists as an abstraction. Failure-path coverage belongs at FilePersistenceProvider level. *(F-40)*

9. **Feature MVP convention + generate.sh + teaching references** — model/presenter/widget structure, composition-root wiring, Counter + AppLog complementary references, generate.sh scaffold, test-generator.sh CI checks. Presenters retained uniformly as teaching product. Do not refactor production for testability — tests already match the architecture well. *(F-41)*

10. **Extension-point documentation honesty** — Live-vs-demo posture: EventSystem subscriptions live; publish side demo-only; AppLog persistence not a production recommendation; custom.qss empty = Qt default; app.env fail-fast documented. Strongest AI-legibility asset. *(F-42)*

11. **cxxopts + fmt as documented Conan dependency demonstrations** — AGENTS.md explicitly documents them and forbids removing cxxopts solely because Qt has QCommandLineParser. Custom code consumes third-party libraries thinly — it does not reinvent parsing/formatting. *(F-43)*

12. **app.env SSOT + fail-fast + scripts facade + CI parity** — Single-file metadata source flowing to Conan, CMake, scripts, VSCode, i18n; FATAL_ERROR / ConanInvalidConfiguration fail-fast; scripts as mandated invocation interface; CI delegating to scripts/build.sh. CMake presets alone cannot replace aqt install, pipenv bootstrap, cygpath, bundle orchestration. *(F-44)*

13. **Build/test infrastructure kept items** — PRIVATE-only linkage (correct for non-exported internal lib); single UnitTests binary + custom gtest main; GLOB_RECURSE + CONFIGURE_DEPENDS with documented fallback; theme via setStyleSheet + Qt 6.5+ colorScheme + custom.qss; apply_app_metadata (verified: no Qt equivalent); `.moc` includes; configureLogLevel → setFilterRules; EventSystem vs diagnostic logging separation; install-qt.sh Windows 7z workaround; ReusableWidget teaching artifact. *(F-45)*

14. **BusRegistry non-owning map + Meyers singleton** — QObject parent-child owns dispatchers/wrappers; registry holds non-owning raw pointers; mutex guards only map mutation. Do not add shared_ptr registry or speculative injection seams. *(F-46)*

15. **Threading model** — Single GUI thread; no QThread/QtConcurrent; EventSystem delivery always queued; BusRegistry mutex creation-only; logging QMutex defensive. Custom code does not fight Qt’s event-loop/thread model. *(implied F-33/F-38)*

---

## G. Proposed Target Architecture

The core architecture is sound. The target is **the same architecture with fixed contracts, cleaned accidental couplings, and coherent API boundaries** — not a disconnected list of refactors.

```text
Target: same architecture, corrected contracts

src/
├── main.cpp                  → compiled into ${APP_NAME} EXE target (not _lib)
│                               apply_app_metadata(${APP_NAME})
├── appmainwindow/            → composition root unchanged (Qt parent-child;
│                               provider first; model+widget before presenter;
│                               contract comment states real Qt guarantees)
├── core/
│   ├── IModel.h              → either polymorphic (model list + closeEvent loop)
│   │                           or removed (convention in AGENTS.md + generator);
│   │                           if kept, "atomic replace" wording, not "durable"
│   └── IPersistenceProvider.h → PersistenceResult kept (C++20); optional
│                                valueOr() helper; key constraints documented
├── features/<name>/
│   ├── model/                → unchanged (required IPersistenceProvider&,
│   │                           load-in-ctor; saveState may return
│   │                           PersistenceResult<void> if F-01 Option A)
│   ├── presenter/            → unchanged (raw non-owning ptrs; dtor discipline:
│   │                           no deref of model/widget pointers)
│   └── widget/               → unchanged
├── events/system/
│   └── EventSystem.hpp       → Event type static_assert strengthened
│                               (default-constructible + copy-assignable);
│                               free-function subscribe uses QCoreApplication
│                               as context (no per-subscribe wrapper);
│                               Subscription holds only QMetaObject::Connection;
│                               dispatcher<T> in detail/ or documented as
│                               diagnostic API; optional services::
│                               unregisterAll(); publish chain const T&
├── services/registry/
│   └── ServiceRegistry.cpp   → registerAll() + unregisterAll(); Subscription
│                               storage pattern documented
├── logging/                  → categories + configureLogLevel unchanged;
│                               messageHandler replaced by qSetMessagePattern
│                               + default handler (or documented demonstration)
├── platform/core/            → FilePersistenceProvider unchanged (QSaveFile)
├── platform/theme/           → unchanged
└── widgets/reusable/         → unchanged (teaching artifact)

tests/
├── features/CounterTest.cpp  → flat layout (docs match reality);
│                               optional dtor-discipline test
├── features/AppLogTest.cpp   → unchanged
├── lifecycle/LifecycleTest.cpp → CloseSavesFeatureModelState added (E2E)
└── main.cpp                  → unchanged (QStandardPaths test mode + QApplication)

CMake/
├── CMakeLists.txt            → main.cpp on exe target; apply_app_metadata on
│                               exe + lib + tests; pick one AUTOMOC/CXX standard story
├── tests/CMakeLists.txt      → autogen include path commented; UT_NAME
│                               hardcoded or documented
└── scripts/conan-lock-update.sh → write temp, mv on success, cleanup

CI/
└── ci.yml                    → optional packaging smoke on PRs; tidy job when
                                capacity allows

Docs/
├── AGENTS.md                 → test paths flat; clang-tidy scope honest; main
│                               layout documented; configuration channels;
│                               install/export; UT_NAME/POST_BUILD discovery;
│                               EventSystem wrapper accumulation + static
│                               teardown nuance; persistence "atomic replace"
│                               wording; destroy-order contract corrected;
│                               layout insertion step; auto-connect convention;
│                               IModel disposition stated
└── README.md                 → project structure tree updated; lock example
                                range fixed
```

The target is coherent: contracts match code; accidental build coupling removed; unused polymorphism adopted or removed; redundant machinery simplified; lifecycle teardown explicit; docs trustworthy. **No subsystem is replaced with a different architecture.**

---

## H. Migration Plan

Ordered by: correctness → lifetime/concurrency safety → obsolete workarounds → unnecessary complexity → API improvements → consistency → cosmetics.

| Phase | Change | Finding | Independent / Coordinated | Complexity | Risk |
|---|---|---|---|---|---|
| **1. Correctness (docs-that-lie + build traps)** | | | | | |
| 1a | Correct AppMainWindow.h destruction-order contract comment | F-02 | Independent | Low | Low |
| 1b | Strengthen EventSystem type static_assert; update contract comment | F-03 | Independent | Low | Low |
| 1c | Fix AGENTS.md + README test-mirror paths to flat layout | F-04 | Independent | Low | Low |
| 1d | Move main.cpp to exe target; apply_app_metadata on exe | F-05 / U-36 | Independent | Low | Low |
| 1e | Fix AGENTS.md clang-tidy claim (editor-side scope); then CI tidy job or curate Checks | F-06 / U-38 | Independent (docs now; CI later) | Low / Medium | Low |
| 1f | Reword IModel.h "durable" to "atomic replace"; fix parse double-log | F-14 | Independent | Low | Low |
| 1g | AGENTS.md feature checklist: layout insertion step (mainLayout->addWidget; do not edit .ui) | F-S2-01 | Independent | Low | Low |
| **2. Lifetime/concurrency safety** | | | | | |
| 2a | Add services::unregisterAll(); call in main on both return paths | F-08 | Independent | Low | Low |
| 2b | Document EventSystem wrapper accumulation; optionally simplify to app-as-context | F-07 / U-02 | Independent (docs now; code optional) | Low | Low |
| 2c | Add teardown-order discipline test | F-15 | Independent | Low | Low |
| 2d | Optional: reorder layout construction so typical Qt destruction matches teaching intent | F-02 alt B | Coordinated with 1a | Low | Low |
| **3. Obsolete workarounds / claim-reality gaps** | | | | | |
| 3a | conan-lock-update: write temp, mv on success, cleanup | F-20 | Independent | Low | Low |
| 3b | Pin aqtinstall to a release; re-verify Windows 7z workaround | F-23 / U-31 | Independent | Low | Low |
| 3c | Align Dockerfile with script path (toolchain-only; drop unused system Qt + Vulkan) | F-24 / U-39 | Independent | Low | Low |
| 3d | Align BUILD_TESTING defaults; document or remove UT_NAME | F-19 / U-40, U-41 | Independent | Low | Low |
| 3e | Document install/export; POST_BUILD discovery; remaining AGENTS.md build facts | F-18 | Independent | Low | Low |
| 3f | Resolve Conan profile vs CI runner alignment | F-48 | Independent | Medium | Medium |
| **4. Unnecessary complexity** | | | | | |
| 4a | Adopt IModel polymorphically (model list + closeEvent loop) OR remove + document | F-09 / U-11 | Independent | Low | Low |
| 4b | Consider replacing messageHandler with qSetMessagePattern (or document as demonstration) | F-11 / U-05 | Independent (careful with fmt Keep) | Low | Low |
| 4c | Narrow dispatcher\<T\> to detail/friend; demote runtime check to debug-only; document boundary | F-10 / U-08 | Coordinated with EventSystem touch points | Low-Medium | Medium |
| 4d | Drop redundant static_cast at composition root | F-29 | Independent | Low | None |
| 4e | Simplify publish chain to const T& (bundle with EventSystem touch) | F-28 | Coordinated with 4c / F-03 / F-07 | Low | Low |
| **5. API improvements** | | | | | |
| 5a | closeEvent observes save results (PersistenceResult\<void\>) OR document log-only policy as explicit invariant | F-01 | Independent | Low | Low |
| 5b | Add valueOr() helper to PersistenceResult | F-26 / U-10 | Independent | Low | Low |
| 5c | Cap AppLogModel loadState at MAX_LOG_SIZE | F-13 | Independent | Low | Low |
| 5d | Document persistence key constraints | F-21 | Independent | Low | Low |
| 5e | Add CloseSavesFeatureModelState E2E test; rename misleading LifecycleTest names | F-12 | Independent | Low | Low |
| **6. Consistency / docs** | | | | | |
| 6a | AGENTS.md: configuration channels (app.env / QSettings chrome / provider feature state) | F-16 | Independent | Low | Low |
| 6b | AGENTS.md: build facts sweep (F-18 list) | F-18 | Independent | Low | Low |
| 6c | README structure tree + lock example fix | F-18 | Independent | Low | Low |
| 6d | AGENTS.md: EventSystem production-status comment hardening; wrapper accumulation + static teardown notes | F-34, F-07, F-08 | Independent | Low | Low |
| 6e | Document auto-connect convention; APP_ID compile-def note; registerAll ordering; model vs provider error layer | F-S2-02/03/04/06 | Independent | Low | Low |
| 6f | Optional: architecture-boundary include checks | F-17 | Independent (optional) | Low-Medium | Low |
| **7. Cosmetics / opportunistic** | | | | | |
| 7a | Comment tests/CMakeLists autogen include path rationale | F-22 | Independent | Low | Low |
| 7b | Widget method naming consistency only if touching files anyway | cosmetic | Independent | Low | Low |
| 7c | CI packaging smoke on labeled PRs (optional) | F-31 | Independent | Low | Low |
| 7d | common.h role one-liner in AGENTS.md | U-21 | Independent | Low | Low |

**Coordination notes:**

- Phases 1–3 are largely independent; **docs-first is always safe**.
- 4c/4e should be one coordinated EventSystem touch alongside F-03 (assert), F-07 (wrapper), and optionally F-08 (unregisterAll) — a single EventSystem.hpp PR.
- 5a touches IModel + models + closeEvent + tests — one coordinated persistence-API PR. If 4a adopts polymorphic IModel, 5a comes naturally.
- 4a (IModel disposition) should be decided before 5a’s API shape, or as part of the same PR.
- **Do not change Keep-list items.** Do not remove EventSystem, cxxopts, fmt, the script façade, app.env SSOT, the MVP convention, or the sync-persistence policy.

---

## I. Final Question

> **If you were designing this template from scratch today, knowing everything discovered during this audit, which parts would you build differently — and which parts would you intentionally build exactly the same way?**

### Would build exactly the same way

- Composition-root Qt parent-child ownership with construction-order encoding
- MV\* feature convention + generator + CI convention checks
- EventSystem as decoupled-events-only facility with always-queued delivery + RAII Subscription + explicit ServiceRegistry (teaching artifact; zero production publishers is intentional)
- IPersistenceProvider + PersistenceResult (C++20 expected-substitute) + MemoryPersistenceProvider seam
- Sync GUI-thread persistence + load-in-ctor + save-in-closeEvent
- app.env SSOT + fail-fast + scripts façade + 3-OS CI parity via scripts
- QSettings for chrome / provider for feature state split
- Theme via setStyleSheet + colorScheme + custom.qss extension point
- QLoggingCategory categories + configureLogLevel
- `.moc` includes + custom gtest main + test-mode QStandardPaths
- No auto-registration macros; no QtConcurrent without requirements; no exceptions in feature code

### Would build differently

1. **main.cpp in the exe target** from day one — apply_app_metadata on the exe; no reliance on static-archive extraction for tests
2. **Honest contracts from day one** — destruction-order comment would state Qt connection semantics + no dtor-time dereference; Event type assert would require default-constructible + assignable; IModel.h would say “atomic replace, not power-loss durability”; AGENTS.md test-mirror paths would be flat; clang-tidy scope documented as editor-side until CI-enforced; feature checklist would include layout insertion
3. **IModel adopted polymorphically** (model list + closeEvent loop) or removed — not left as an unused marker contradicting non-goals
4. **Free-function Subscription without per-subscribe wrapper QObject** — app-as-context from the start; Subscription holds only the connection handle
5. **services::unregisterAll() in main** as part of the service-registration pattern (deterministic teardown taught, not incidental static-destruction timing)
6. **closeEvent observes save results** (PersistenceResult\<void\> return) or the log-only policy is an explicit documented invariant — not a silent composition-root blind spot
7. **BUILD_TESTING single default**; UT_NAME hardcoded or documented; messageHandler replaced by qSetMessagePattern (or documented as a qInstallMessageHandler demonstration like cxxopts)
8. **Dockerfile aligned with script path** (toolchain-only; no parallel system Qt; no Vulkan packages contradicting AGENTS.md)
9. **aqtinstall pinned to a release** from the start; lock-update writes temp and mv on success
10. **Architecture-boundary tests** (include-pattern checks) if the template is expected to be extended by many agents

**Would remain a deliberate opinionated template, not a “best practices” showcase** — the audit principle that intentional opinions must be preserved unless evidence contradicts them is correct and should remain the extension philosophy.

---

## Success Criteria Answers (AUDIT.md)

**1. What architecture does this template actually implement?**  
An opinionated Qt 6 application template: composition-root AppMainWindow owns feature object graphs via Qt parent-child; MV\* features with Qt signals/slots intra-feature and an optional typed queued EventSystem bus for genuinely decoupled cross-component events; explicit typed persistence errors via IPersistenceProvider + PersistenceResult\<T\> with a testability seam; synchronous GUI-thread persistence (load-in-ctor, save-in-closeEvent); QSettings for window chrome; app.env metadata SSOT with fail-fast across Conan/CMake/scripts; generator + CI-checked feature conventions; three-OS build/CI/packaging via scripts façade.

**2. Which design decisions are deliberate and justified?**  
Composition-root Qt parent-child ownership; MV\* convention + presenters as teaching uniformity; EventSystem reserved for decoupled events with always-queued delivery + RAII Subscription + explicit ServiceRegistry; LogEvent demonstration posture; PersistenceResult under C++20; sync GUI-thread persistence; dual persistence channels; cxxopts/fmt as Conan dependency demos; app.env SSOT + fail-fast + scripts + CI parity; no auto-registration macros; no exceptions in feature code; no QtConcurrent without requirements; custom.qss empty = Qt default. *(All Keep findings.)*

**3. Which abstractions are unnecessary?**  
IModel as an unused polymorphic marker (contradicts non-goals); custom messageHandler formatting machinery (largely redundant with qSetMessagePattern — categories + configureLogLevel are canonical and worth keeping); free-function subscribe wrapper QObject (app-as-context achieves same semantics); friend-class test declarations (currently unused; harmless); UT_NAME indirection; triple BUILD_TESTING defaults.

**4. Which abstractions provide meaningful value?**  
EventSystem + Subscription (open typed topics, always-queued, RAII free-function subscribe that Qt does not package as one facility; test-locked); IPersistenceProvider + PersistenceResult (typed error taxonomy + testability seam; correct C++20 expected-substitute); MemoryPersistenceProvider; app.env SSOT + scripts façade (3-OS CI parity real); generate.sh + test-generator.sh; apply_app_metadata (verified: no Qt facility provides metadata compile defs); theme loading (canonical setStyleSheet + colorScheme); custom gtest main; Conan lock merge strategy.

**5. Where is the repository fighting Qt rather than using Qt?**  
Almost nowhere. Investigation leads closed as canonical-mechanism-used-correctly or justified-custom-abstraction: EventSystem wraps Qt queued connections; theme uses Qt 6.5+ colorScheme + canonical setStyleSheet; QSettings geometry is canonical; `.moc` includes are AUTOMOC requirement; configureLogLevel→setFilterRules is canonical; apply_app_metadata verified against Qt 6.11 docs (qt_add_executable has no metadata args). The closest to “fighting” is defensive custom machinery whose justification depends on an undocumented API boundary (U-08 D: runtime type check + public dispatcher exposure).

**6. Where is it fighting C++ rather than using C++?**  
Nowhere materially. PersistenceResult is the correct C++20 expected-substitute (std::expected is C++23; project pins gnu20). Modern C++ posture is sound: RAII Subscription, optional/variant for result channels, concepts for overload constraints; no cargo-cult shared_ptr/ranges/premature expected. Minor nits: publish by-value+move chain before mandatory QVariant copy (F-28); redundant static_cast upcast (F-29). Abort-on-misuse PersistenceResult accessors is a deliberate fail-fast contract, not fighting C++.

**7. Which mechanisms are historical workarounds?**  
Three D-class classifications: main.cpp compiled into static lib (accidental coupling to static-archive extraction); runtime QVariant type check paired with public dispatcher exposure (defensive machinery for an abuse path the public API creates); clang-tidy WarningsAsErrors claimed as build invariant but enforced editor-only (claim-reality gap). None are historical Qt workarounds in the “we worked around Qt X for reason Y” sense — they are accidental coupling and documentation overclaims. **Zero E classifications.**

**8. Which mechanisms should be replaced with canonical Qt/C++ facilities?**  
Custom messageHandler formatting → qSetMessagePattern + default handler (keep categories + configureLogLevel). Free-function subscribe wrapper → QCoreApplication as connection context. IModel unused polymorphism → adopt polymorphically or remove. main.cpp in static lib → exe target. Everything else investigated as potential replacements closed as NOT worth replacing — canonical alternatives either unavailable (C++23), lose required semantics (error taxonomy, test seam, open typed topics), or contradict documented intentional opinions.

**9. Which apparently unusual mechanisms should actually be retained?**  
EventSystem bus + Subscription RAII + always-queued delivery; ServiceRegistry explicit registration without macros; LogEvent zero-production-publisher demonstration posture; PersistenceResult abort-on-misuse contract; QObject parenting of FilePersistenceProvider (structurally safer than unique_ptr); sync GUI-thread persistence; dual QSettings/provider persistence channels; cxxopts + fmt as Conan demos; app.env SSOT + fail-fast; scripts façade; custom gtest main; `.moc` includes; install-qt.sh Windows 7z workaround; GLOB_RECURSE + CONFIGURE_DEPENDS; ReusableWidget teaching artifact; apply_app_metadata; theme colorScheme + custom.qss; MemoryPersistenceProvider seam; BusRegistry Meyers singleton + non-owning map; presenter layer as uniform teaching policy; EventSystem vs diagnostic logging separation.

**10. What should be changed now?**  
Phase 1 correctness: F-02, F-03, F-04, F-05, F-06, F-14, F-S2-01. Phase 2 lifetime: F-08, F-07, F-15. Phase 3 workaround cleanup: F-20, F-23, F-24, F-19, F-18. Then F-09, F-11, F-01, F-12.

**11. What should explicitly NOT be changed?**  
Everything on the Keep List (section F): EventSystem architecture + always-queued + Subscription RAII + explicit ServiceRegistry; LogEvent demonstration posture; composition-root Qt parent-child ownership + provider QObject parenting + IPersistenceProvider& required ref + presenter raw pointers; PersistenceResult C++20 expected-substitute + FilePersistenceProvider QSaveFile + sync persistence + dual channels + load-in-ctor/save-in-closeEvent; MemoryPersistenceProvider seam; MVP convention + generate.sh + Counter/AppLog references + presenters as uniform teaching policy; extension-point documentation honesty; cxxopts + fmt as documented Conan demos; app.env SSOT + fail-fast + scripts façade + CI parity; PRIVATE linkage + single UnitTests + custom gtest main + GLOB_RECURSE + theme canonical APIs + apply_app_metadata + `.moc` includes + 7z workaround + ReusableWidget; BusRegistry non-owning map + Meyers singleton; threading model; no auto-registration macros; no exceptions in feature code; custom.qss empty = Qt default; String Boundary Policy.

**12. What should be reconsidered only if the template’s requirements change?**  
C++23 adoption → migrate PersistenceResult to std::expected (mechanical; even then evaluate keeping the no-exception misuse contract). Dynamic free-function subscription patterns → revisit wrapper accumulation (app-as-context already solves). Isolated event buses → revisit BusRegistry injection seam. Second test binary → revisit UT_NAME. Merge conflicts in conan.lock → per-platform lockfiles. Large-state consumers → revisit sync persistence per six-step async escalation. Multiple services → revisit receivedCount vs injectable sink. Windows-debug console determinism as stated CI requirement → reconsider messageHandler retention. aqt upstream fix for py7zr bug → drop install-qt 7z workaround with evidence. Qt upgrades overriding CMAKE_CXX_STANDARD via qt_standard_project_setup → re-check F-25 duplication. Consumer apps graduating from template wanting Qt-native CLI/format → migrate cxxopts/fmt deliberately. Architecture-boundary enforcement → add include-pattern tests when template will see many agent-driven extensions.

**13. What would make the architecture easier for both humans and AI coding agents to extend correctly?**  
Fix the contract-strength defects (F-02, F-03, F-04, F-06, F-14, F-S2-01): agents follow documented invariants; false guarantees are worse than missing docs. Make error semantics observable at the composition root (F-01) or document the log-only policy explicitly. Teach deterministic teardown (F-08 unregisterAll + document static-destruction nuance). Resolve IModel disposition (F-09). Document configuration channels (F-16). Harden EventSystem production-status comments at subscription sites (F-34). Optionally enforce architecture-boundary rules with lightweight include checks (F-17). Keep the generator + CI convention checks (F-41). Preserve the honest live-vs-demo documentation posture (F-42).

---

## Appendix — Quality Gate Notes

### Findings rejected or downgraded during synthesis

1. **“EventSystem is dead infrastructure”** — rejected as removal basis; accepted as doc-hardening note. Evidence is documentation + tests, not contradiction. Resolution: Keep (F-34) + one-line comment hardening.
2. **“cxxopts/fmt are redundant with Qt”** — downgraded from candidate C to documented intentional opinion B. AGENTS.md explicitly documents both and forbids removing cxxopts solely because Qt has an equivalent.
3. **“PersistenceResult is a historical std::expected workaround”** — rejected. C++23 unavailable under pinned C++20; misuse contract differs even under C++23; taxonomy load-bearing.
4. **“IModel should be removed because unused”** — downgraded from removal-default to polymorphic-adoption-preferred. Classification C remains; recommendation is adoption-first, removal-fallback.
5. **“messageHandler is actively harmful”** — downgraded to Improvement/Class C redundancy. No correctness bug.
6. **“Static Subscription teardown is Critical UB”** — downgraded to Improvement. QMetaObject::Connection is a weak handle; disconnect on stale state is a Qt no-op. Safe under normal order; untested in that order.
7. **“unique_ptr ownership would be more modern”** — rejected. unique_ptr members are destroyed in member destruction, which runs BEFORE ~QObject children — provider would die before models, inverting desired order.
8. **“Presenters are unnecessary for Counter-like features”** — acknowledged but not recommended for removal. Uniform presenter convention is the teaching product; AppLog demonstrates presenter-layer value.
9. **Friend-class white-box scaffolding** — B3 classified B; B5 read tests directly and found no private-member access. Resolution: C (currently unused; harmless).
10. **xorg/system in conan.lock not declared in conanfile.py** — noted as evidence for F-48 Investigate; not elevated to standalone finding.

### Investigate items and resolution evidence needed

| ID | Question | Evidence that would resolve |
|---|---|---|
| F-47 | Is cross-subscriber delivery order part of the template’s contract? | Maintainer policy decision; optional EventsTest locking multi-subscriber order |
| F-48 | Do Conan profiles pin compilers that match CI runners? Is xorg/system transitive or residue? | Compare profile settings to CI runner toolchains; lock refresh inspection |

### Method note

This audit was performed by parallel explore/general sub-agents covering architecture reconstruction, EventSystem deep-dive, persistence/error handling, Qt6 un-workaround sweep, ownership/modern C++, features/ergonomics, and build/CI — then a findings synthesizer, an AI-ergonomics verifier, and a quality-gate pass that downgraded over-inflated findings against evidence. Qt 6 API comparisons were verified against documentation where classification depended on current Qt capability (notably `std::expected` = C++23; `qt_add_executable` metadata args absent; `QStyleHints::colorScheme`; `QSaveFile` atomicity; QObject object-tree destruction-order docs; context-object connect semantics).

**End of report.**
