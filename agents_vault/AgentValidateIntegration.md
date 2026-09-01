---
type: agent
tags: [agent, sc-agent, integration, validation]
action_class: action_validate_integration
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentValidateIntegration

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_validate_integration`, READY) ·
covers part of `AgentVerifyResult`'s scope from AGENTS_AUDIT.md §7.5

> [!note] A slice of `AgentVerifyResult`, not the whole thing
> AGENTS_AUDIT.md §7.5 lists a single `AgentVerifyResult` that "checks
> constraints, explanation, provenance and conflicts." That is four
> different rule families. This agent implements exactly one of them —
> `rule_validate_integration` (explanation + conflicts) — the same way
> [[AgentValidateCardinalityConstraint]] covers only the constraint part
> and [[AgentComputePiMetric]] only the provenance part. No single agent
> in the module is named `AgentVerifyResult`.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_validate_integration` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentValidateIntegration : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_validate_integration.{hpp,cpp}` |
| Role | Implements `rule_validate_integration` (`agent_result_integrator` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_integration.scs` → `rule_validate_integration` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** Before an integrated result `ir` is committed, the rule
requires two guarantees: it has an explanation, and it is not in conflict
with anything. Until [[AgentBuildExplanation]] existed, `nrel_explained_by`
was only ever set by [[AgentResolveByTrustWeighted]]'s own inline
placeholder — so a check for condition (a) would have been checking
something no general-purpose agent actually produced. Now that
[[AgentBuildExplanation]] sets `nrel_explained_by` for any explanandum
classified `concept_integrated_result`, this validation gate has real
data to check.

**Goal.** Given `ir`, decide `valid = ∃e. explained_by(ir, e) ∧
¬∃c. conflicts_with(ir, c)` and write it as a boolean indicator.

**Value.** A reusable, cheap admissibility gate before publication — the
last of the three "ready" checks named across `rules_integration.scs` and
`rules_integrity_validation.scs` (the other two being cardinality/existence
shapes, already covered by [[AgentValidateCardinalityConstraint]]).

**Stakeholders:** the not-yet-implemented `AgentPublishResult` (§7.5),
which per the audit's minimal readiness checklist "must not publish
before verification, explanation and absence of unresolved conflict" —
this agent computes exactly that precondition.

## Competence

**Capabilities:**
- check `∃e. explained_by(ir, e)` via a single outgoing-arc lookup;
- check `¬∃c. conflicts_with(ir, c)` in **both** directions — outgoing
  and incoming — because `nrel_conflicts_with` is declared symmetric
  (`relations/relations_trust.scs`) but is not always materialized as two
  arcs in existing KB scenarios (`scenario_collective_decision.scs` sets
  both directions; `scenario_smart_city_traffic.scs` sets only one).
  Checking only the outgoing direction would silently miss the latter
  shape;
- write the combined boolean as `1.0`/`0.0` into a `value_link` on
  `result_level`, creating it on first access (same convention as
  [[AgentDispatchReadySubtask]]/[[AgentDetectAnomalousRequestRate]]);
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not act on failure** — the rule text says "otherwise return
  control to `rule_resolve_by_*`"; the agent computes the indicator only,
  it does not initiate any other action or agent. Same limitation class
  as [[AgentDetectAnomalousRequestRate]] not activating
  `agent_coalition_reconfigurator` — sequencing across agents is the
  external orchestrator's job, which the project does not yet have.
- **does not cover the rest of `AgentVerifyResult`'s named scope** —
  constraints and provenance are separately (and independently) covered
  by [[AgentValidateCardinalityConstraint]] and [[AgentComputePiMetric]];
  this agent does not call or duplicate either.
- **does not validate that `integrated_result` is actually classified
  `concept_integrated_result`** — only `IsElement`, trusting the caller,
  same convention as the rest of the module.
- **a conflict resolved and later cleared is treated as no conflict** —
  the agent has no notion of `nrel_conflict_status`; it looks only at
  raw `nrel_conflicts_with` arcs. If a conflict was resolved by
  [[AgentResolveByTrustWeighted]] but the `conflicts_with` arc itself was
  never erased, this agent would (correctly, per the literal rule text)
  still report `invalid` — erasing resolved `conflicts_with` arcs is
  nobody's job in the current codebase.

## Input

**Trigger:** arc `action_initiated -> <instance of action_validate_integration>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `integrated_result` | `ScAddr` of the `concept_integrated_result` instance to validate | required | only `IsElement` |
| 2 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level` with
`1.0` (valid) or `0.0` (invalid). Success — `FinishSuccessfully()` +
`FormResult(integratedResult, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`keynodes/integration_keynodes.hpp` (`IntegrationKeynodes` — extended
with `nrel_conflicts_with` and `action_validate_integration`, both
new to this file; `nrel_explained_by`/`concept_integrated_result` already
existed there from [[AgentResolveByTrustWeighted]]).

**Reads:** outgoing `nrel_explained_by` from `integrated_result`; outgoing
and incoming `nrel_conflicts_with` on `integrated_result`.

**Writes:** the `value_link` of `result_level`.

**Workflow.** Reads exactly what [[AgentBuildExplanation]] and
[[AgentResolveByTrustWeighted]] write (`nrel_explained_by`) and what
`rule_detect_conflict` would write (`nrel_conflicts_with`, not itself
implemented by any agent in the project — only present in KB scenarios as
fixture data). Not wired to `AgentPublishResult`, which does not exist.

## Behavior

```
IF  integrated_result is not an sc-element  OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  has_explanation := ∃ outgoing ConstCommonArc from integrated_result
                      marked nrel_explained_by
  has_conflict    := (∃ outgoing ConstCommonArc from integrated_result
                       marked nrel_conflicts_with)
                      OR (∃ incoming ConstCommonArc to integrated_result
                          marked nrel_conflicts_with)

  valid := has_explanation AND NOT has_conflict

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (valid ? 1.0 : 0.0) to value_link
  action.FormResult(integrated_result, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument;
absence of an explanation or presence of a conflict is a normal,
non-error outcome (`valid = false`).

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the agent trusts that `nrel_conflicts_with`/
  `nrel_explained_by` arcs in sc-memory accurately reflect reality — it
  has no way to detect a stale `conflicts_with` arc left over from an
  already-resolved conflict (see Limitations), which would make a
  genuinely valid result report as invalid, or the reverse if such an arc
  were maliciously removed by a caller with write access.
- The indicator itself carries no PII; it is a boolean gate over
  existing structural links.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — one iterator for `explained_by`, two (outgoing + incoming) for `conflicts_with`, independent of KB size. |
| Accuracy | Not applicable — deterministic boolean combination; verified by tests including the asymmetric-materialization case. |
| Cost | Negligible. |
| Alerts | None — the indicator is a passive, queryable value; nothing consumes it proactively (`AgentPublishResult`, its natural consumer, does not exist). |

## Tests

File: `tests/test_agent_validate_integration.cpp` (4 tests, all pass).

**1. `ValidatesSuccessfullyWhenExplainedAndConflictFree`**
Input: `ir` with `nrel_explained_by` set, no conflicts.
Output: indicator `1.0`, `FinishedSuccessfully`.

**2. `InvalidWhenNoExplanation`**
Input: `ir` with no `nrel_explained_by`, no conflicts.
Output: indicator `0.0` — missing explanation alone is enough to fail.

**3. `InvalidWhenConflictOnlyIncoming`**
Input: `ir` with `nrel_explained_by` set, and a conflict materialized only
as an incoming arc (`competitor -[nrel_conflicts_with]-> ir`), mirroring
`scenario_smart_city_traffic.scs`.
Output: indicator `0.0` — confirms the incoming-direction check is not
dead code.

**4. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
