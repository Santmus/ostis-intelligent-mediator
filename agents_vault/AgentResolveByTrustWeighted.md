---
type: agent
tags: [agent, sc-agent, integration, argmax]
action_class: action_resolve_by_trust_weighted
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentResolveByTrustWeighted

← [[Agent Index]] · from [[Implementation Backlog]] (top-5, #3) · reuses [[Trust Model|trust(s,τ,h)]]

> [!note] The only agent that reads an already computed trust
> All other six agents either *compute* a trust component or do not know
> about trust at all. This is the only one that **consumes** the already
> calculated `trust(s,τ,h)` ([[AgentAggregateTrust]]) as a ready input
> signal for another decision (choosing the winner among conflicting
> results).

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_resolve_by_trust_weighted` |
| sc-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentResolveByTrustWeighted : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_resolve_by_trust_weighted.{hpp,cpp}` |
| Role | Implements `rule_resolve_by_trust_weighted` (part of `agent_result_integrator`/`agent_conflict_detector`, `rules_integration.scs`) |
| Knowledge-base rule | `rules/rules_integration.scs` → `rule_resolve_by_trust_weighted` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] — the same argument about one module per `.dylib`) |

## Purpose

**Problem.** Multiple intelligent systems may produce mutually
contradictory results for the same subtask (`concept_result_conflict` —
semantic inconsistency, logical contradiction, etc., see
`concepts/interactions.scs`). One resolution method is not argumentation,
but simple source-trust voting.

**Goal.** Among a set of conflicting results, choose the one produced by
the system with the maximum `trust(s,τ,h)`, record the outcome as
`concept_integrated_result`, and formally resolve the conflict through a
reified `concept_conflict_resolution_act` plus an explanation, rather
than by pointing `nrel_resolves_conflict` at the result directly (a
divergence from the implementation flagged in
`relations/relations_trust.scs` and closed by this fix).

**Value.** A cheap, fast (without building an argumentative framework, as
in `rule_resolve_by_argumentation`) conflict-resolution heuristic — and
at the same time it closes the loop: trust accumulated by
[[AgentUpdateTrustReliability|EMA agents]] and aggregated by
[[AgentAggregateTrust|the aggregator]] actually influences whose result
the system ultimately prefers.

**Stakeholders:** `agent_result_integrator`/`agent_conflict_detector`
(not implemented in full, only this part), end users receiving a single
consistent answer instead of contradictory ones.

## Competence

**Capabilities:**
- for each candidate in the given group find the producing system
  (`nrel_produced_by`) and its trust level (`nrel_has_trust_level` →
  `value_link`);
- select the candidate with the maximum weight (`arg max_i w_i`);
- create `concept_integrated_result`, link it to the winner via
  `nrel_integrated_in`;
- create a `concept_conflict_resolution_act`, specialized as
  `concept_resolution_by_trust_weighting`, and link the passed conflict
  to it via `nrel_resolved_by`; move the conflict's `nrel_conflict_status`
  (functional relation) to `concept_conflict_status_resolved`;
- create a `concept_explanation`, link it to the conflict via
  `nrel_resolves_conflict` and to the integrated result via
  `nrel_explained_by`;
- terminate correctly with an error when arguments are missing or the
  candidate group is empty.

**Limitations (from the code):**
- **the explanation carries no textual justification** ("with
  justification" in the rule text) — the created `concept_explanation`
  node is structurally correct (typed, linked to both the conflict and
  the integrated result) but empty of content; filling it in is
  essentially the task of `agent_explanation_builder`, not implemented
  in the project — only a debug message with the final weight remains in
  the log.
- **a candidate without a trust chain gets weight 0**, not an
  error/exception — indistinguishable from a system with genuinely zero
  trust (the same principle as in [[AgentAggregateTrust]]).
- **does not verify** that the passed candidates actually conflict with
  each other or that `conflict` refers specifically to them — fully
  trusts the caller (no reading of
  `nrel_conflicts_with`/`concept_result_conflict` content for
  verification).
- **tie-break is not specified**: when weights are equal (including "all
  zeros"), the first one found while traversing the candidate group's
  arcs wins — the traversal order is not documented by the sc-machine
  API as deterministic.
- unlike metric agents, **an empty group is an error**, not a neutral
  result: there is nothing to choose from, so this is the only agent in
  the project where "empty" is not treated as 0.

## Input

**Trigger:** arc `action_initiated -> <instance of action_resolve_by_trust_weighted>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `candidates_group` | node with outgoing `ConstPermPosArc`s to conflicting `concept_processing_result`s | required | `IsElement`; empty group → error |
| 2 | `conflict` | existing instance of `concept_result_conflict` | required | `IsElement` only |

## Output

A new instance of `concept_integrated_result` (ir), a new
`concept_conflict_resolution_act` (act, specialized as
`concept_resolution_by_trust_weighting`), and a new `concept_explanation`
(expl), linked:
- `r* -[nrel_integrated_in]-> ir` (winning candidate),
- `conflict -[nrel_resolved_by]-> act`,
- `conflict -[nrel_conflict_status]-> concept_conflict_status_resolved`
  (functional — any prior status arc is erased),
- `expl -[nrel_resolves_conflict]-> conflict`,
- `ir -[nrel_explained_by]-> expl`.

`FormResult(ir, r*, act, expl)`. Side effect — generation of all three
new nodes and the five links above (`ConstCommonArc` + classifying
`ConstPermPosArc` from the relation, following the standard pattern for
non-role binary relations in sc-machine; the status link uses
`SetFunctionalRelationTarget`).

## Links

**Dependencies:** `common/float_link_io.hpp`
(`ReadFloatLinkContent`), `common/relation_io.hpp`
(`TargetOf`/`LinkByRelation`/`SetFunctionalRelationTarget` — when
[[AgentDispatchReadySubtask]] appeared and these same functions were
needed a second time, they were moved from a local copy into a common
header), `keynodes/integration_keynodes.hpp` (`IntegrationKeynodes`,
extended with `nrel_resolved_by`, `nrel_conflict_status`,
`nrel_explained_by`, `concept_conflict_resolution_act`,
`concept_resolution_by_trust_weighting`,
`concept_conflict_status_resolved`, `concept_explanation`).

**Reads:** `nrel_produced_by` of each candidate,
`nrel_has_trust_level` of the producing system, `value_link` of the
found `trust_level` — the same node that [[AgentAggregateTrust]] writes
to.

**Writes:** new `concept_integrated_result`, new
`concept_conflict_resolution_act`, new `concept_explanation`, and the
five links listed under Output.

**Workflow.** The only agent formally dependent on the **result** of
[[AgentAggregateTrust]] (through data — it reads the same `trust_level`
that the aggregator populates), although technically it does not call it
and does not check that trust has already been calculated for the
required τ.

## Behavior

```
IF  candidates_group is not an sc-element  OR  conflict is not an sc-element
THEN  FinishWithError()

ELSE
  best := ∅; best_weight := -∞
  FOR EACH candidate IN candidates_group (by ConstPermPosArc):
    system := TargetOf(candidate, nrel_produced_by)         // 0 if none
    trust_level := TargetOf(system, nrel_has_trust_level)   // 0 if none
    w := value of value_link trust_level, OTHERWISE 0
    IF  w > best_weight
    THEN  best_weight := w; best := candidate

  IF  best == ∅  (group was empty)
  THEN  FinishWithError()

  ELSE
    ir := create a node of class concept_integrated_result
    link best -[nrel_integrated_in]-> ir

    act := create a node of class concept_conflict_resolution_act,
           also classified concept_resolution_by_trust_weighting
    link conflict -[nrel_resolved_by]-> act
    set conflict -[nrel_conflict_status]-> concept_conflict_status_resolved
      // functional relation: prior status arc is erased

    expl := create a node of class concept_explanation
    link expl -[nrel_resolves_conflict]-> conflict
    link ir -[nrel_explained_by]-> expl

    action.FormResult(ir, best, act, expl)
    → action.FinishSuccessfully()
```

**Error/timeout handling:** as for the other agents — `DoProgram` is
synchronous, waiting on the caller side via `InitiateAndWait`.

## Security

- Privileges — same as the other `TrustModule` agents: no ACL, full
  access to sc-memory within the `sc-server` process.
- **Specific risk:** because a candidate without a trust chain silently
  gets weight 0 rather than being rejected, a deliberately constructed
  "empty" candidate will never win fairly — but it will not be excluded
  from consideration with an explicit warning either; in a system with
  mostly untrusted participants (all weights ≈0) the winner is selected
  according to an undocumented traversal order, which is not a
  meaningful decision but looks like one.
- The agent does not verify the caller's right to resolve a conflict —
  it trusts any source of `action_initiated`.
- The data (trust, results) are not PII, but `concept_integrated_result`
  can become the basis for decisions affecting users
  (`concept_collective_decision <= nrel_inclusion: concept_integrated_result`).
- The generated `concept_explanation` has no textual content — a
  consumer expecting a human-readable justification for the resolution
  will find only a structurally linked, empty node.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; linear in the number of candidates — up to 3 link traversals per candidate (`nrel_produced_by`, `nrel_has_trust_level`, `value_link`). |
| Accuracy | Not applicable to the formula itself (argmax is deterministic); correctness is verified by tests, including an explicit check that the highest weight wins rather than insertion order. |
| Cost | Negligible at typical sizes of conflicting-result groups (usually single digits). |
| Alerts | Not implemented. An alert for "winner with weight 0" would be appropriate — a signal that none of the candidates has a real trust history. |

## Tests

File: `tests/test_agent_resolve_by_trust_weighted.cpp` (3 tests, all
passing; scenario 1 additionally verified on a live sc-server).

**1. `PicksResultFromHighestTrustSystem`**
Input: three candidates with trust 0.60, 0.95, 0.80 (insertion order
deliberately not ascending/descending).
Output: the candidate with 0.95 wins (not the first or last inserted);
`concept_integrated_result` created and linked from the winner via
`nrel_integrated_in`; the conflict is linked to a
`concept_resolution_by_trust_weighting` act via `nrel_resolved_by` and
its `nrel_conflict_status` is `concept_conflict_status_resolved`; a
`concept_explanation` is linked to the conflict via
`nrel_resolves_conflict` and to the integrated result via
`nrel_explained_by`; all four created nodes (`ir`, `r*`, `act`, `expl`)
are present in the action result.

**2. `TreatsCandidateWithoutTrustChainAsZero`**
Input: one candidate without `nrel_produced_by`, another with trust 0.10.
Output: the candidate with 0.10 wins — confirms the Limitation "no trust
chain = weight 0", not an error.

**3. `FinishesWithErrorOnEmptyCandidatesGroup`**
Input: empty candidate group.
Output: `FinishedWithError` — unlike metric agents, an empty input here
is not 0 but an error.
