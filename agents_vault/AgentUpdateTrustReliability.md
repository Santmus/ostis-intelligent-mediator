---
type: agent
tags: [agent, trust-agent, sc-agent, ema]
action_class: action_update_trust_reliability
version: 72ba122 (2026-08-18)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentUpdateTrustReliability

← [[Agent Index]] · component of [[Trust Model|t_rel]]

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_update_trust_reliability` |
| sc-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentUpdateTrustReliability : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_update_trust_reliability.{hpp,cpp}` |
| Role | Executor of rule `rule_update_t_rel_on_outcome` (π_tr) — a reactive sc-agent passively waiting for an action of its class to be initiated |
| KB rule | `rules/rules_trust_update.scs` → `rule_update_t_rel_on_outcome` |
| Version | commit `72ba122` (first and current implementation; no changes since the initial commit) |

## Purpose

**Problem.** Trust in an intelligent system `s` cannot be set once by hand
and forgotten — it must reflect the *actual* interaction history, not a
static label. The `t_rel` component specifically handles «availability and
correctness of responses» — the most basic question: «does this system
respond stably at all and not crash or err?».

**Goal.** After every interaction with system `s`, record whether the
outcome was successful (ν_rel ∈ {0,1}) and shift the accumulated `t_rel`
score toward it — not instantly (a single failure should not wipe out
reputation), but not infinitely slowly either (steady degradation should
be noticeable after a reasonable number of interactions).

**Value.** Gives the rest of the system (primarily the executor-selection
function `match(σ, s)`) an up-to-date, self-updating signal — eliminating
manual trust calibration and sharp jumps from a single random failure.

**Stakeholders:**
- orchestrators/subtask dispatchers (consumers of `t_rel` via `match`);
- ecosystem developers and administrators (monitoring system degradation);
- the intelligent systems `s` themselves — `t_rel` determines how much work will be entrusted to them;
- end users — indirectly, through the quality of task routing.

## Competence

**Capabilities (what it can do):**
- from the pair (a `trust_level` node, an sc-link with the indicator)
  compute a new exponentially smoothed `t_rel` value;
- create a numeric sc-link for `trust_level` if it does not yet exist
  (neutral start `t_rel = 0`);
- return the updated `trust_level` and `value_link` in the action result;
- complete correctly with an error on invalid arguments, without leaving
  the action «stuck».

**Limitations (what it cannot do — verified from the code):**
- does **not verify** that the first argument is actually an instance of
  `concept_trust_level` — the code only does
  `m_context.IsElement(trustLevelAddr)`. Any existing sc-node will be
  silently accepted, and a numeric sc-link will be attached to it.
- does not check the indicator range: `ν_rel` can be any number parseable
  by `std::stof` (including negative or > 1) — then `t_rel` will exceed
  the ontologically expected range [0,1] without error.
- does not write provenance: there is no link to the specific interaction/
  subtask that triggered the update — from sc-memory alone it is impossible
  to reconstruct «why `t_rel` changed at that exact moment» without an
  external log.
- does not adapt the coefficient: α = 0.10 is a constant in the code,
  uniform for all systems and contexts; it can only be changed by
  rebuilding.
- does not perform any side effects other than writing the value (does
  not send notifications, does not write to the log except for
  `m_logger.Debug`).

## Input

**Trigger.** Event `ScEventAfterGenerateOutgoingArc` on the
`action_initiated` subscription element — i.e., generating the arc
`action_initiated -> <instance of action_update_trust_reliability>`
(`ConstPermPosArc`). The usual way is to call `ScAction::Initiate()` /
`InitiateAndWait()` after `SetArguments(...)`.

**Action parameters (both mandatory, no optional ones):**

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `trust_level` | `ScAddr` of any sc-element | required | only `IsElement` |
| 2 | `outcome_link` | `ScAddr` of an sc-link | required | `IsElement` + `GetLinkContent` + `std::stof` does not throw |

**Format:** the indicator is an sc-link with **textual** numeric content,
e.g. `"1.0"` or `"0"` (not raw float bytes).

## Output

**Result.** The updated (or newly created) numeric sc-link `value_link`
attached to `trust_level` by a `ConstPermPosArc`; its content is
overwritten to `std::to_string(newTRel)` (format like `"0.838000"`).

**Side effects:** if `trust_level` had no sc-link — a new node
(`ScType::ConstNodeLink`) and the arc connecting it are generated.

**Success criterion:** `action.FinishSuccessfully()` and
`action.FormResult(trustLevelAddr, valueLinkAddr)` — the result structure
contains both nodes. Checked in tests via `IsFinishedSuccessfully()` +
reading the updated link content.

## Relations

**Dependencies (code):**
- `common/float_link_io.hpp` — `trust_utils::ReadFloatLinkContent` / `WriteFloatLinkContent`.
- `keynodes/trust_keynodes.hpp` — `TrustKeynodes::concept_trust_level`, `action_update_trust_reliability`.
- sc-machine: `ScActionInitiatedAgent`, `ScAction`, `ScAgentContext`.

**Reads:** classification of `trust_level` (implicitly — only by trying to
find its sc-link), current content of `value_link` (if any), content of
`outcome_link`.

**Writes:** content of `value_link` (creates it if missing + the
connecting arc).

**Workflow.** One of four independent «leaf» nodes of the π_tr pipeline —
does not depend on [[AgentUpdateTrustAccuracy]] /
[[AgentUpdateTrustSecurity]] / [[AgentUpdateTrustTransparency]], but its
result (the `t_rel` value) is read by [[AgentAggregateTrust]] during
aggregation.

## Behavior

```
IF  trust_level is not an sc-element  OR  outcome_link is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE IF  content of outcome_link does not parse as a number
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  currentTRel := value of value_link if it exists for trust_level, ELSE 0
  IF value_link did not exist
  THEN  create value_link (ConstNodeLink) + arc trust_level -> value_link

  newTRel := (1 - α) * currentTRel + α * outcome     // α = 0.10
  write newTRel into value_link (as text)
  action.FormResult(trust_level, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** both failures (missing arguments / not a number)
produce the same `action.FinishWithError()` — from the outside (by action
status) they are indistinguishable; they can only be distinguished by the
sc-server log (`AgentUpdateTrustReliability: action does not have both
arguments.` vs `...outcome argument is not a numeric sc-link.`).

**Timeouts:** the agent itself has no internal timeout — `DoProgram` is
synchronous and completes in one pass without waits/loops. The timeout is
set by the **calling side** via
`InitiateAndWait(maxCustomerWaitingTime)` (2000 ms in tests); it is
implemented in `ScAction`/`ScWaiterActionFinished` of sc-machine itself,
not in this agent.

## Security

- **Rights:** the agent runs inside the `sc-server` process with the same
  privileges as any other loaded extension — there is no separate
  sandbox/ACL at the agent level in this project; technically it can
  read/create/erase any sc-element, not only those related to trust.
- **Limitations:** because membership in `concept_trust_level` is not
  checked (see Limitations), the caller may pass an arbitrary node, and
  the agent will silently assign it a numeric value — this is a real
  data-integrity risk on incorrect invocation, not merely theoretical.
- **Sensitive data:** `t_rel` itself is an abstract scalar and contains
  no PII. Risk exists only if `trust_level` is linked in another part of
  the ontology to an identifiable entity (a specific user/organization);
  personal data should not be written directly into this value or its
  sc-link.
- **Authentication/authorization:** the agent does not check who initiated
  the action — it trusts any source of the `action_initiated` arc; access
  control (if needed) must be on the side of whatever generates that arc
  (not implemented in this project).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous operation with no I/O: 1 iterator + 0–2 generations + 1–2 reads/writes of sc-elements. In gtest the whole test case (including sc-memory init/shutdown) is 9–19 ms; the `DoProgram` code itself is orders of magnitude faster; no separate benchmark was run. |
| Accuracy | Not applicable in the ML sense — deterministic arithmetic, not a model. «Accuracy» = correctness of the formula, verified by unit tests with manual calculation of the expected value. |
| Cost | Negligible: no external calls, no GPU/API, only sc-memory operations. |
| Alerts | **Not implemented.** No observability for crossing critical thresholds (e.g., a sharp drop in `t_rel`). Future recommendation — a separate agent/subscription to the `value_link` change event, generating an alert when a threshold is crossed. |

## Tests

File: `tests/test_agent_update_trust_reliability.cpp` (3 tests, all pass;
the 2nd and part of the 1st were additionally verified manually on a live
sc-server).

**1. `UpdatesTRelOnSuccessfulOutcome`**
Input: `trust_level` with current `t_rel = 0.82`, `outcome_link = 1.0`.
Expected output: `t_rel_new = 0.9·0.82 + 0.1·1.0 = 0.838`, action
`FinishedSuccessfully`.

**2. `CreatesValueLinkWhenMissing`**
Input: `trust_level` without a numeric sc-link, `outcome_link = 1.0`.
Expected output: a new sc-link is created with value
`0.9·0 + 0.1·1.0 = 0.10`, action `FinishedSuccessfully`.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments (`SetArguments()` with no parameters).
Expected output: action `FinishedWithError`, no value is created.
