---
type: agent
tags: [agent, trust-agent, sc-agent, ema]
action_class: action_update_trust_transparency
version: 72ba122 (2026-08-18)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentUpdateTrustTransparency

← [[Agent Index]] · component of [[Trust Model|t_tr]] · pattern same as [[AgentUpdateTrustReliability]]

> [!warning] `ms_delta` → `η` pending — see [[Chapter 2 Alignment]]
> Chapter 2 as revised 2026-08-24 uses a single smoothing coefficient `η` for
> all four components instead of α/β/γ/δ. The value is unchanged (`0.10`), so
> nothing computes differently; the rename is a naming fix, and `η` should also
> become an action argument so it can be calibrated without recompiling.
> Not yet applied — everything below describes the current code.
>
> The indicator `ν_tr ∈ {0, 1}` was checked against the revised chapter
> (`part2.tex` line 262) and **matches** — no change needed there.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of action class | `action_update_trust_transparency` |
| sc-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentUpdateTrustTransparency : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_update_trust_transparency.{hpp,cpp}` |
| Role | Executor of rule `rule_update_t_tr_on_justification` (π_tr) |
| KB rule | `rules/rules_trust_update.scs` → `rule_update_t_tr_on_justification` |
| Version | commit `72ba122` |

## Purpose

**Problem.** Explainability is not a static architectural property of a
system ("can explain / cannot explain"), but an *observed behavior*:
how often the system actually accompanies a result with a justification
when one is requested. This is a separate, historical, empirical
metric — `t_tr` (see the definition of `concept_trust_component_transparency`
in `concepts/trust.scs`: "does not substitute explanation e as part of a
concrete result").

**Goal.** After each justification request, record whether it was
actually provided (`ν_tr ∈ {0,1}`) and smooth `t_tr` toward that value.

**Value.** Distinguishes systems that declare support for explanations
from systems that *actually* provide them on request — important in
domains requiring decision auditability (healthcare, finance, law).

**Stakeholders:** users who need a justification for a decision;
auditors/regulators; orchestrators choosing between systems when
explainability is required.

## Competence

**Capabilities:**
- given a pair (`trust_level` node, sc-link with `ν_tr`), compute the
  exponentially smoothed new value of `t_tr`;
- create an sc-link on first access (neutral start `t_tr = 0`);
- return updated `trust_level`/`value_link`;
- correctly finish with error on invalid arguments.

**Limitations (by code, identical to [[AgentUpdateTrustReliability]]):**
- does not check whether `trust_level` belongs to class `concept_trust_level`;
- does not check the range of `ν_tr`;
- does not write provenance (which specific justification request produced this indicator);
- δ = 0.10 — constant in code;
- **does not assess the quality of the justification itself** — only the binary fact
  "provided / not provided"; a boilerplate dismissal and a substantive
  explanation are indistinguishable from this agent's perspective.

## Input

**Trigger:** arc `action_initiated -> <instance of action_update_trust_transparency>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `trust_level` | `ScAddr` | required | only `IsElement` |
| 2 | `justification_link` | `ScAddr` sc-link with `ν_tr` | required | `IsElement` + parsed by `std::stof` |

**Format:** textual sc-link, `"1.0"` (justification provided) or `"0.0"`.

## Output

Updated/created sc-link `value_link` for `trust_level`. Side effect —
generation of `value_link` + arc on first call. Success —
`FinishSuccessfully()` + `FormResult(trustLevel, valueLink)`.

## Connections

Identical to other EMA agents: `common/float_link_io.hpp`,
`TrustKeynodes`. Independent of [[AgentUpdateTrustReliability]] /
[[AgentUpdateTrustAccuracy]] / [[AgentUpdateTrustSecurity]]; the result is
read by [[AgentAggregateTrust]].

## Behavior

```
IF  trust_level is not an sc-element  OR  justification_link is not an sc-element
THEN  FinishWithError()

ELSE IF  justification_link content is not a number
THEN  FinishWithError()

ELSE
  currentTTr := value of value_link if exists, ELSE 0 (+ create if missing)
  newTTr := (1 - δ) * currentTTr + δ * ν_tr          // δ = 0.10
  write newTTr, FormResult(...), FinishSuccessfully()
```

Error handling/timeouts — same as [[AgentUpdateTrustReliability]].

## Security

Identical to other EMA agents: full privileges of the `sc-server`
process, without class check of `trust_level`. `ν_tr` is an abstract
indicator, contains no PII.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O. |
| Accuracy | Not applicable to the formula; correctness verified by tests. |
| Cost | Negligible. |
| Alerts | Not implemented. |

## Tests

File: `tests/test_agent_update_trust_transparency.cpp` (3 tests, all
passing; scenario 1 additionally verified on a live sc-server).

**1. `UpdatesTTrOnJustificationProvided`**
Input: `t_tr = 0.60`, `ν_tr = 1` (justification provided).
Output: `t_tr_new = 0.9·0.60 + 0.1·1.0 = 0.64`, `FinishedSuccessfully`.

**2. `CreatesValueLinkWhenMissing`**
Input: `trust_level` without an sc-link, `ν_tr = 1`.
Output: link created with value `0.9·0 + 0.1·1 = 0.10`.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action with no arguments.
Output: `FinishedWithError`.
