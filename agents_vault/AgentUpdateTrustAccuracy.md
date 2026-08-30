---
type: agent
tags: [agent, trust-agent, sc-agent, ema]
action_class: action_update_trust_accuracy
version: 72ba122 (2026-08-18)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentUpdateTrustAccuracy

← [[Agent Index]] · component of [[Trust Model|t_acc]] · same pattern as [[AgentUpdateTrustReliability]]

> [!warning] `ms_beta` → `η` pending — see [[Chapter 2 Alignment]]
> Chapter 2 as revised 2026-08-24 uses a single smoothing coefficient `η` for
> all four components instead of α/β/γ/δ. The value is unchanged (`0.10`), so
> nothing computes differently; the rename is a naming fix, and `η` should also
> become an action argument so it can be calibrated without recompiling.
> Not yet applied — everything below describes the current code.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_update_trust_accuracy` |
| Class sc-node type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentUpdateTrustAccuracy : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_update_trust_accuracy.{hpp,cpp}` |
| Role | Executor of rule `rule_update_t_acc_on_quality` (π_tr) |
| KB rule | `rules/rules_trust_update.scs` → `rule_update_t_acc_on_quality` |
| Version | commit `72ba122` |

## Purpose

**Problem.** `t_rel` ([[AgentUpdateTrustReliability]]) only handles the
success/failure fact, not the *quality* of a successful result. Two
systems may equally “not fail” but produce answers of different quality —
this is a separate trust component `t_acc` (“accuracy”), which tracks
quality metrics on test/historical data.

**Goal.** After each processed subtask, record the achieved result
accuracy `q_res ∈ [0,1]` (unlike binary ν_rel, this is a continuous
quality metric) and smooth the accumulated `t_acc` estimate toward it.

**Value.** Allows `match(σ, s)` to distinguish not just
“works/doesn’t work” but “works and produces a quality result” — important
where only systems above a certain accuracy threshold are acceptable
(e.g., medical diagnosis, financial decisions).

**Stakeholders:** same as [[AgentUpdateTrustReliability]], plus —
primarily domains with strict result-quality requirements (see
`scenario_medical_diagnostics.scs`, `scenario_financial_decision_support.scs`
in [[ostis_kb_user_ecosystem]]).

## Competence

**Capabilities:**
- for a pair (node `trust_level`, sc-link with `q_res`) compute the
  exponentially smoothed new value of `t_acc`;
- create an sc-link on first access (neutral start `t_acc = 0`);
- return the updated `trust_level`/`value_link` in the action result;
- correctly finish with an error on invalid arguments.

**Limitations (by code, identical to [[AgentUpdateTrustReliability]]):**
- does not check whether the first argument belongs to `concept_trust_level` —
  only `IsElement`;
- does not check that `q_res` is actually ∈ [0,1] — accepts any number
  parseable by `std::stof`;
- does not write provenance (which specific subtask produced this `q_res`);
- β = 0.10 — a constant in code, not configurable by context/system;
- in addition to common limitations: **does not determine the method of
  measuring `q_res`** — accuracy calculation (against ground truth, by
  labeling, by an ensemble judge, etc.) is fully outside the agent’s
  responsibility; it only accepts an already prepared number.

## Input

**Trigger:** arc `action_initiated -> <instance of action_update_trust_accuracy>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `trust_level` | `ScAddr` | required | only `IsElement` |
| 2 | `quality_link` | `ScAddr` sc-link with `q_res` | required | `IsElement` + parseable by `std::stof` |

**Format:** `quality_link` — a textual sc-link, e.g. `"0.9"`.

## Output

Updated/created sc-link `value_link` on `trust_level` with content
`std::to_string(newTAcc)`. Side effect — generating `value_link` + a
binding arc on the first call. Success —
`FinishSuccessfully()` + `FormResult(trustLevel, valueLink)`.

## Relations

Dependencies, read/write — identical to [[AgentUpdateTrustReliability]]
(common `common/float_link_io.hpp`, `TrustKeynodes`). Independent of the
other three EMA agents; the result is read by [[AgentAggregateTrust]].

## Behavior

```
IF  trust_level is not an sc-element  OR  quality_link is not an sc-element
THEN  FinishWithError()

ELSE IF  quality_link content is not a number
THEN  FinishWithError()

ELSE
  currentTAcc := value of value_link if present, OTHERWISE 0 (+ create if missing)
  newTAcc := (1 - β) * currentTAcc + β * q_res       // β = 0.10
  write newTAcc, FormResult(...), FinishSuccessfully()
```

Error handling and timeouts — same as [[AgentUpdateTrustReliability]]
(the caller sets the timeout via `InitiateAndWait`; the agent itself has
no internal waiting).

## Security

Identical to [[AgentUpdateTrustReliability]]: full privileges of the
`sc-server` process, no ACL at the agent level; the same risk — an
arbitrary node may be accepted as `trust_level` without class check. `q_res`
is an abstract quality metric and contains no PII.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Same characteristic as [[AgentUpdateTrustReliability]] — synchronous, no I/O. |
| Accuracy | Not applicable (deterministic formula); correctness verified by tests. |
| Cost | Negligible. |
| Alerts | Not implemented. |

## Tests

File: `tests/test_agent_update_trust_accuracy.cpp` (3 tests, all pass; scenario 1 additionally verified on a live sc-server).

**1. `UpdatesTAccOnQualityResult`**
Input: `t_acc = 0.70`, `q_res = 0.90`.
Output: `t_acc_new = 0.9·0.70 + 0.1·0.90 = 0.72`, `FinishedSuccessfully`.

**2. `CreatesValueLinkWhenMissing`**
Input: `trust_level` without an sc-link, `q_res = 1.0`.
Output: a link is created with value `0.9·0 + 0.1·1.0 = 0.10`.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
