# Qt6 Application Template — Next Architectural Remediation Proposal

## 1. Purpose

This proposal addresses the findings from the 2026-09-30 architectural audit.

The audit's central conclusion is accepted:

> **The architecture is sound. The remaining problems are primarily contract-strength defects, lifecycle incidentalism, build coupling, documentation drift, and a small amount of unnecessary machinery.**

The objective of this remediation is therefore **not** to redesign the template.

The objective is to make the existing architecture:

* mechanically correct,
* honest about its guarantees,
* deterministic at lifecycle boundaries,
* internally consistent across code/generator/docs/CI,
* easier for humans and AI agents to extend correctly,
* and free of accidental historical machinery where removal is clearly justified.

The audit explicitly found **zero E-class actively harmful mechanisms** and no classic historical "fighting Qt" workaround. Do not manufacture architectural churn in response to this audit.

---

# 2. Architectural Decisions

These decisions are binding for this remediation.

## 2.1 EventSystem / LogEvent

**Keep the documented dormant-demo posture.**

Do not invent a production publisher for `LogEvent`.

The stock application intentionally demonstrates:

```text
EventSystem subscription
        ↓
LogEvent
        ↓
AppLog / ConsoleLogService
```

without routing diagnostic logging through the EventSystem.

Diagnostic logging remains:

```text
qC* → Qt message handling → stdout/stderr
```

The absence of a production `LogEvent` publisher is intentional pedagogy, not dead infrastructure.

Add a concise comment at the relevant subscription site so an agent does not mistake the lack of publishers for an unfinished feature.

---

## 2.2 EventSystem teardown

**Use explicit application-lifecycle teardown, not arbitrary global destruction.**

Implement:

```cpp
services::unregisterAll();
```

and use it from `main()` before `QApplication` is destroyed.

The service registration API should therefore teach:

```text
QApplication exists
    ↓
registerAll()
    ↓
application runs
    ↓
unregisterAll()
    ↓
QApplication destruction
```

Do not introduce a generic:

```cpp
events::shutdown();
```

unless the implementation proves that service-level unregistration cannot establish the required lifetime boundary.

The EventSystem itself remains application-scoped. We are fixing incidental static-destruction behavior rather than creating a second global lifecycle API.

---

## 2.3 Provider ownership

**Retain QObject parent-child ownership.**

Do not convert `FilePersistenceProvider` to `unique_ptr` merely because it is a dependency.

The audit's current conclusion is that the existing composition-root ownership model is structurally sound and, for this QObject graph, preferable to introducing an independent ownership mechanism.

Preserve:

```text
AppMainWindow
 ├── FilePersistenceProvider
 ├── models
 ├── widgets
 └── presenters
```

with the provider constructed before models and the dependency encoded explicitly.

However, **fix the presenter destruction-order contract**. Construction order must not be documented as a Qt guarantee about arbitrary QObject destruction order.

The actual rule should be:

> Presenter pointers are non-owning. Presenter destructors must not dereference model/widget pointers. Signal/slot connections provide automatic disconnection when either QObject is destroyed.

---

## 2.4 Persistence failure semantics

**Choose explicit propagation rather than silently discarding the result.**

This is the one place where I would go slightly beyond the audit's minimum acceptable "document the log-only behavior" option.

Change:

```cpp
virtual void saveState() = 0;
```

to:

```cpp
virtual PersistenceResult<void> saveState() = 0;
```

and propagate the provider result through the models.

`AppMainWindow::closeEvent()` should then explicitly observe the results.

Do **not** turn this into a modal "don't let the user close" workflow yet. The first objective is simply that the composition root knows whether persistence succeeded.

The initial policy may still be:

```text
save
 ↓
failure?
 ↓
qCWarning / diagnostic
 ↓
continue shutdown
```

but the API must make that policy explicit rather than making save failure invisible to the composition root.

This resolves the audit's AD-02 debt: the repository can truthfully say that persistence errors are explicit throughout the persistence path, while retaining the deliberate log-and-continue shutdown policy.

---

# 3. Wave 1 — Correctness and Contract Repair

Implement these first and **stop for review afterward**.

## 3.1 Fix presenter lifetime documentation — F-02

Change the `AppMainWindow` ownership comments.

Remove any implication that:

> "constructed before" ⇒ "guaranteed to be destroyed after."

Document instead:

* Qt parent-child ownership determines ownership.
* Presenter pointers are non-owning.
* QObject signal connections automatically disconnect when either endpoint dies.
* Presenter destructors must not dereference `m_model` or `m_view`.
* Any teardown logic requiring those objects must occur before destruction, not in presenter destructors.

Do not add `QPointer` merely for defensive aesthetics.

Do not redesign the ownership graph.

### Optional structural cleanup

If convenient, arrange construction/reparenting so normal Qt destruction also happens in the intuitive order, but this is secondary.

The **contract correction is mandatory**; relying on typical destruction order is not.

---

## 3.2 Strengthen EventSystem's type contract — F-03

The public contract currently says event types only need to be copy-constructible, while implementation requires additional operations.

Make the compile-time requirements match reality.

At minimum require:

```cpp
std::is_default_constructible_v<T>
std::is_copy_assignable_v<T>
```

alongside the existing requirements.

Update the documentation accordingly.

Prefer a concept if the existing C++20 style makes that cleaner:

```cpp
template <typename T>
concept EventType =
    std::default_initializable<T> &&
    std::copy_constructible<T> &&
    std::assignable_from<T&, const T&>;
```

Do not over-engineer this into a general serialization framework.

The goal is simply:

> If an event type satisfies the documented contract, it should compile through the EventSystem implementation.

---

## 3.3 Fix documentation's test layout — F-04

The actual layout is:

```text
tests/features/CounterTest.cpp
tests/features/AppLogTest.cpp
```

not:

```text
tests/features/Counter/
tests/features/AppLog/
```

Update:

* `AGENTS.md`
* `README.md`
* any generator documentation

to describe the actual flat structure.

Do not restructure the test tree merely to make stale documentation true.

---

## 3.4 Move `main.cpp` to the executable target — F-05

This is the one build-architecture change I consider clearly worthwhile.

Current:

```text
${APP_NAME}_lib
    └── main.cpp
${APP_NAME}
    └── effectively no application TU
```

Change to:

```text
${APP_NAME}_lib
    └── application/library sources

${APP_NAME}
    └── src/main.cpp
```

Apply:

```cmake
apply_app_metadata(${APP_NAME})
```

to the executable target.

Tests continue linking against `${APP_NAME}_lib`.

This removes the accidental dependency on static archive extraction behavior and makes the target graph communicate what it actually is.

Do not turn this into a broader CMake rewrite.

---

## 3.5 Fix clang-tidy claim — F-06

Immediately correct `AGENTS.md` so it does **not** claim that clang-tidy warnings are CI/build errors unless they actually are.

For this wave:

* accurately document the current editor-side enforcement;
* preserve `.clang-tidy`;
* preserve existing intentional `NOLINT` annotations.

Do **not** add a large clang-tidy CI system during Wave 1.

A later CI-quality wave may either:

1. add a real clang-tidy job, or
2. curate the enabled checks and explicitly define clang-tidy as editor-side policy.

The important immediate fix is that the documentation must stop claiming enforcement that does not exist.

---

## 3.6 Correct persistence terminology — F-14

Replace claims of power-loss durability with precise terminology.

If the implementation uses `QSaveFile` for atomic replacement, document:

> atomic replacement

not:

> durable before returning

unless actual filesystem durability guarantees are explicitly established.

Also eliminate parse-error double logging.

There should be one clear owner for logging each persistence failure.

---

## 3.7 Add layout insertion to the feature checklist — F-S2-01

This is a high-value AI-agent fix.

The canonical feature workflow must explicitly include:

```text
Generate feature
↓
Inspect generated files
↓
Wire model/widget/presenter
↓
Inject persistence provider
↓
Connect signals
↓
Add widget to AppMainWindow's central layout
↓
Add persistence to closeEvent
↓
Add tests
↓
Build/test
```

Explicitly state:

> The central layout is code-built in `AppMainWindow`; add feature widgets with `mainLayout->addWidget(...)`. Do not add feature widgets to `AppMainWindow.ui`.

This closes one of the most dangerous silent extension failures in the entire audit.

---

# 4. Wave 1 Validation

Before committing Wave 1, run all of:

### Build

* clean Linux build
* clean Windows/macOS builds if available through CI
* renamed `APP_NAME` build

### Tests

* complete CTest suite
* EventSystem tests
* persistence tests
* lifecycle tests
* generator smoke test

### Structural validation

Verify:

* `${APP_NAME}` owns `main.cpp`
* `${APP_NAME}_lib` contains no `main()`
* test target links library without relying on archive extraction
* generated EventSystem event types satisfy the actual contract
* no stale test-directory documentation remains
* no documentation claims clang-tidy is CI-enforced

### Git

Commit Wave 1 as:

> `refactor: repair architectural contracts and build target boundaries`

Then **STOP**.

Do not begin Wave 2 without review.

---

# 5. Wave 2 — Lifecycle and EventSystem Cleanup

After Wave 1 approval:

## 5.1 Explicit service teardown — F-08

Implement:

```cpp
services::registerAll();
...
services::unregisterAll();
```

`unregisterAll()` must actually reset/disconnect retained subscriptions.

The operation must be:

* idempotent,
* safe to call once,
* safe to call during normal shutdown,
* independent of static destructor timing.

Test:

1. registration works;
2. event is received;
3. unregister stops reception;
4. repeated unregister is harmless;
5. application can subsequently destroy EventSystem objects cleanly.

The static subscription should no longer be relied upon as the lifecycle mechanism.

---

## 5.2 Simplify free-function subscription — F-07 / U-02

The audit identifies the per-subscription wrapper QObject as unnecessary accumulation.

Change free-function subscription to use:

```cpp
QCoreApplication::instance()
```

as the connection context.

`Subscription` should contain the `QMetaObject::Connection`, not a dedicated wrapper QObject.

Conceptually:

```text
Current:

Subscription
 └── wrapper QObject
      └── connection

Target:

Subscription
 └── QMetaObject::Connection
       ↓
QCoreApplication context
```

This removes:

* one QObject allocation per free-function subscription,
* wrapper accumulation,
* incidental wrapper destruction timing.

Retain `Subscription` itself. The RAII abstraction is valuable.

---

## 5.3 Teardown-order test — F-15

Add a test fixture specifically designed to destroy:

```text
view
model
presenter
```

in the relevant teardown order.

The test should establish the intended rule:

> presenter destruction must not require its non-owning dependencies to remain alive.

Do not write a test that falsely claims to establish a Qt guarantee that Qt does not provide.

---

## 5.4 EventSystem behavioral contract

Document explicitly:

* delivery is always queued;
* events may be dropped when the receiver is destroyed;
* publishers do not synchronously execute subscribers;
* subscriber ordering follows the chosen documented Qt semantics;
* subscriptions must be retained;
* `reset()` disconnects;
* destroyed QObject receivers are automatically disconnected;
* free-function subscriptions belong to application lifetime;
* publishing is a GUI-thread operation under the current architecture;
* recursive publication is queued rather than immediate.

For cross-subscriber ordering, make a conscious decision:

**Preferred:** treat connection-order delivery as an implementation/Qt-derived property rather than a stronger application guarantee unless you want to lock it with a regression test.

Do not promise more ordering than the application actually needs.

---

# 6. Wave 3 — Build and Reproducibility Cleanup

## 6.1 Conan lock update hazard — F-20

Change the lock update workflow from:

```text
delete existing lock
↓
merge
```

to:

```text
create temporary lock
↓
perform merge/update
↓
success?
 ├── yes → atomic mv/replace
 └── no  → preserve existing lock
```

Always clean temporary artifacts.

The existing lock must never disappear merely because a merge failed.

---

## 6.2 Pin aqtinstall — F-23

Replace the git-master dependency with a known release.

Then explicitly validate the existing Windows 7z workaround against that version.

Do not remove the workaround merely because the dependency is pinned; remove it only if the underlying upstream issue is demonstrably fixed.

---

## 6.3 Docker toolchain alignment — F-24

The Docker environment should teach the same toolchain path as the supported scripts.

Remove parallel system Qt/Vulkan installation if it is genuinely unused by the supported build path.

The desired invariant:

```text
Developer
CI
Docker
```

should all communicate the same Qt acquisition/build model.

Do not add Vulkan merely to make Docker's current state match itself.

---

## 6.4 Build configuration defaults — F-19

Eliminate the three competing `BUILD_TESTING` defaults.

Choose one authoritative default.

My preference:

> `BUILD_TESTING=ON` through the supported developer/build-script path.

Document any direct-CMake difference if one remains.

For `UT_NAME`, either:

* hardcode the single test binary name, or
* explicitly document it as an intentional customization point.

For a template with one test binary, I lean toward removing unnecessary indirection.

---

# 7. Wave 4 — Persistence API

This wave deliberately follows the earlier EventSystem/lifecycle work.

## 7.1 Make `saveState()` return `PersistenceResult<void>`

Change:

```cpp
void saveState();
```

to:

```cpp
PersistenceResult<void> saveState();
```

for `IModel` and implementations.

Models should forward provider results rather than flattening them immediately.

Example conceptual flow:

```text
FilePersistenceProvider
        ↓
PersistenceResult<void>
        ↓
Model::saveState()
        ↓
PersistenceResult<void>
        ↓
AppMainWindow::closeEvent()
```

The composition root decides the shutdown policy.

---

## 7.2 Preserve log-and-continue policy

Do **not** make persistence failure block shutdown by default.

The initial policy remains:

```text
save failure
    ↓
diagnostic warning
    ↓
continue closing
```

But now the policy is explicit and testable.

This is materially different from silently discarding the result.

Future products can choose:

```text
save failure
    ↓
user notification
    ↓
retry / cancel / continue
```

without having to redesign the persistence interface.

---

## 7.3 Harden PersistenceResult

Keep the C++20 `PersistenceResult`.

Do not migrate to C++23 merely to obtain `std::expected`.

The current taxonomy remains meaningful:

```text
NotFound
IoError
InvalidData
DurabilityFailure
```

with `NotFound` intentionally representing first-run state where appropriate.

Private storage is preferred if not already established.

Keep the fail-fast misuse policy unless a concrete ergonomic problem appears.

---

# 8. Wave 5 — AI-Agent Contract and Documentation

This wave is intentionally substantial because the audit identifies documentation as one of the template's most important architectural surfaces.

## AGENTS.md must explicitly document

### Ownership

* AppMainWindow is composition root.
* QObject children are owned by parents.
* Provider is a QObject child.
* presenter pointers are non-owning.
* presenters must not dereference dependencies during destruction.
* `Subscription` owns its connection lifetime.

### EventSystem

* use only for genuinely decoupled events;
* use signals/slots for direct feature relationships;
* never use EventSystem for diagnostic logging;
* always queued;
* retain returned `Subscription`;
* free-function subscription lifetime;
* QObject receiver lifetime;
* delivery/drop semantics.

### Persistence

* provider errors are explicit;
* model behavior;
* composition-root behavior;
* atomic replacement versus durability;
* QSettings = application/window chrome;
* persistence provider = feature state;
* `app.env` = build metadata.

### Feature generation

Explicitly require:

```text
generate
→ inspect
→ wire
→ layout
→ persistence
→ test
```

### Designer

Document the `on_<object>_<signal>()` convention if it remains part of the template.

### Service registration

Document:

```text
QApplication
→ registerAll()
→ application
→ unregisterAll()
→ QApplication destruction
```

### Async work

Keep the existing deliberate non-goal:

> There is no background-worker architecture.

But give the positive escalation path:

```text
1. Establish measured workload
2. Identify operation boundary
3. Define worker ownership
4. Define result delivery
5. Define cancellation
6. Define shutdown semantics
7. Only then introduce QThread/QtConcurrent
```

---

# 9. Wave 6 — Architectural Boundary Enforcement

The audit correctly identifies a remaining distinction:

> some architecture rules are documented but not mechanically enforced.

Add lightweight CI checks where they have high value.

Examples:

### Widget dependency rule

Prevent feature widgets from including model headers.

### Diagnostic/event separation

Prevent production logging files from publishing `LogEvent`.

### Layering

Prevent obvious dependency inversions.

These should be simple source-pattern checks, not a home-grown static-analysis framework.

Do this only after the documentation contract is stable.

---

# 10. Deliberately Do NOT Change

The following are explicitly outside the remediation target.

## EventSystem

Keep:

* typed event bus;
* always-queued semantics;
* RAII `Subscription`;
* open typed topics;
* QVariant erasure;
* BusRegistry;
* QObject receiver subscriptions;
* free-function subscriptions.

The audit's conclusion is that these semantics represent a legitimate abstraction beyond ordinary Qt signals.

## LogEvent

Keep the dormant demonstration.

Do not invent a publisher.

## PersistenceResult

Keep the C++20 implementation.

Do not import `std::expected` or a polyfill.

## QObject ownership

Keep the composition-root parent-child model.

Do not blanket-convert to `unique_ptr`.

## MVP / Presenter

Keep the uniform Model/Presenter/Widget convention.

Do not eliminate presenters merely because one feature has a thin presenter.

## cxxopts + fmt

Keep them as documented template dependency demonstrations.

Do not replace them with Qt-native alternatives merely because equivalents exist.

## app.env

Keep it as the metadata SSOT.

Do not reintroduce fallback metadata values.

## GLOB_RECURSE

Keep `CONFIGURE_DEPENDS`.

Do not replace it with manually maintained source lists without a concrete problem.

## Synchronous persistence

Keep it synchronous.

Do not introduce workers merely to make the architecture look "modern."

## Custom GTest main

Keep it.

It is required for the template's QApplication and QStandardPaths setup.

## Theme

Keep:

```text
QStyleHints::colorScheme()
+
setStyleSheet()
+
custom.qss
```

An empty custom stylesheet is valid and should be documented as such.

---

# 11. Final Validation

After all waves:

### Architecture

* no undocumented ownership assumptions;
* no false destruction guarantees;
* no hidden static-destruction lifecycle;
* no silent persistence-result discard;
* EventSystem contract exactly matches implementation.

### Generator

Generate a feature from scratch.

Then verify:

* generated code compiles;
* generated persistence matches reference;
* generated presenter lifetime comments match reference;
* generated tests match current test layout;
* generated code does not reintroduce obsolete patterns.

### Build

Test:

* Linux
* Windows
* macOS
* renamed `APP_NAME`
* clean build
* incremental build
* test build
* Docker build if supported

### Tests

Run:

```text
ctest --output-on-failure
```

and verify actual execution rather than merely successful discovery/configuration.

### Lifecycle

Test:

```text
register
→ publish
→ receive
→ unregister
→ publish
→ no receive
→ QApplication teardown
```

### Persistence

Test:

* first run / NotFound;
* successful load;
* successful save;
* malformed data;
* I/O failure;
* save failure at close;
* close continues after save failure;
* result reaches composition root.

### Documentation

Perform a final mechanical search for stale claims:

```text
MyApp
features/<name>/
clang-tidy warnings are errors
durable
destroy order
main.cpp in library
```

Any remaining occurrence must either be correct or intentionally contextualized.

---

# 12. Definition of Done

This remediation is complete when:

1. **Code guarantees match documentation.**
2. **Generator output matches the reference implementation.**
3. **The build graph expresses the actual application architecture.**
4. **Service teardown is explicit.**
5. **Persistence errors reach the composition root.**
6. **EventSystem type requirements are enforced at compile time.**
7. **Free-function subscription no longer allocates an unnecessary wrapper QObject.**
8. **Tests reproduce meaningful lifecycle behavior.**
9. **CI verifies tests rather than merely building them.**
10. **Documentation describes actual file/layout conventions.**
11. **No intentional architecture from the Keep List has been replaced merely because it is unusual.**
12. **A freshly generated feature follows the same architecture as the hand-written reference feature.**

The desired end state is not a more sophisticated framework.

It is:

> **One obvious way to do the right thing, with the code, generator, tests, CI, and documentation all teaching the same thing.**

---

# 13. Execution Order

The agent must execute in this order:

```text
WAVE 1
  F-02  presenter contract
  F-03  EventSystem type contract
  F-04  test layout documentation
  F-05  main.cpp → executable
  F-06  clang-tidy claim
  F-14  persistence terminology
  F-S2-01 feature layout checklist
        ↓
STOP FOR REVIEW
        ↓
WAVE 2
  F-08  explicit service teardown
  F-07  subscription wrapper simplification
  F-15  teardown tests
  EventSystem contract hardening
        ↓
WAVE 3
  F-20  lock update safety
  F-23  aqt pin
  F-24  Docker alignment
  F-19  build defaults
  F-18  documentation sweep
        ↓
WAVE 4
  F-01  PersistenceResult propagation
  F-09  IModel disposition
  F-13  AppLog persistence invariant
        ↓
WAVE 5
  F-10  dispatcher API boundary
  F-11  messageHandler simplification investigation
  F-17  architecture-boundary checks
  remaining AI-agent contract work
        ↓
FINAL AUDIT
```

## Important execution rule

**Do not bundle unrelated cleanup into earlier waves.**

Every wave should leave the repository in a buildable/testable state.

After Wave 1, stop and present:

* changed files;
* exact changes;
* tests run;
* build configurations tested;
* generator result;
* any deviations from this proposal;
* commit hash.

Do not proceed to Wave 2 until explicitly approved.
