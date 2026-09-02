---
type: agent
tags: [agent, sc-agent, user-adaptation, provenance, graph-traversal]
action_class: action_evaluate_progressive_disclosure
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentEvaluateProgressiveDisclosure

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_progressive_disclosure`, READY) ·
first agent for `rules_user_adaptation.scs` (F_rule_adapt)

> [!note] Decides, does not present
> The rule's consequent is "the adaptive interface presents the top
> level (executive summary)" — no agent in the project creates a
> `concept_adaptive_interface` instance, and this one does not either.
> It only computes the two-threshold condition and reports the decision
> as a boolean indicator; whatever downstream component eventually
> renders the interface is expected to read it.

> [!warning] Indicator polarity is inverted relative to the rest of the module
> Every `AgentValidate*` agent in the project writes `1.0` for "no
> violation" / "condition satisfied". This agent writes `1.0` for
> "the rule fired — progressive disclosure IS needed", the literal
> value of the rule's own boolean, not a validity judgment. Read the
> code/tests before assuming the usual polarity.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_evaluate_progressive_disclosure` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentEvaluateProgressiveDisclosure : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_evaluate_progressive_disclosure.{hpp,cpp}` |
| Role | Implements the computable part of `rule_progressive_disclosure` (F_rule_adapt, `rules/rules_user_adaptation.scs`) |
| Knowledge base rule | `rules/rules_user_adaptation.scs` → `rule_progressive_disclosure` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `concept_strategy_progressive_disclosure`
(`concepts/adaptive_interface.scs`) names the strategy, but nothing
computed the two-threshold condition ("provenance depth > 4 steps OR
partial result count > 5") that is supposed to trigger it.

**Goal.** Given an explanation (or any node with an outgoing
`nrel_derived_from` chain) and a group of partial results, compute
whether either threshold is exceeded.

**Value.** Turns the rule's boolean condition into an executable check
over already-typed relations (`nrel_derived_from`, group membership),
the numeric-threshold sibling of [[AgentDetectAnomalousRequestRate]]'s
`rate > k_σ·baseline` pattern, applied to a graph metric instead of a
scalar counter.

**Stakeholders:** whatever future adaptive-interface component would
read this indicator to decide rendering (not implemented, see
Limitations) — the same downstream gap as `AgentPublishResult` for
[[AgentValidateRightToExplanation]].

## Competence

**Capabilities:**
- compute the depth of the longest chain of outgoing
  `nrel_derived_from` arcs from `explanation` via a memoized DFS
  (0 for a node with no outgoing `derived_from`);
- count the direct children (`ConstPermPosArc`) of `partial_results_group`
  — the same `elements_group` convention [[AgentComputePiMetric]]
  established for a caller-assembled container, reused here verbatim;
- report `needsProgressiveDisclosure = depth > 4 || count > 5` as
  `1.0`/`0.0` in a `value_link` on `result_level`;
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not present anything** — no `concept_adaptive_interface`
  instance is taken as an argument or written to; "адаптивный интерфейс
  представляет верхний уровень" has no materialization here (see the
  note above).
- **does not write `nrel_applies_strategy`** — same reason: nothing in
  the project creates the `concept_adaptive_interface` source node that
  relation requires.
- **domain observation, not fixed here:** `nrel_derived_from` is
  declared `concept_processing_result → concept_processing_result`
  (`relations/relations_provenance.scs`), not `concept_explanation`;
  the agent walks whatever node is passed regardless of class, the same
  trust-the-caller stance already documented for
  [[AgentValidateProvenanceChain]]'s identical domain tension.
- **`partial_results_group` membership is not class-checked** — any
  `ConstPermPosArc` child counts, whether or not it is actually a
  `concept_partial_result`; identical trust level to
  [[AgentComputePiMetric]]'s `elements_group`.
- **inverted indicator polarity** relative to the `AgentValidate*`
  family (see the warning above) — a deliberate, documented departure,
  not an inconsistency to fix.
- only `IsElement` on arguments, same convention as the rest of the
  module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_evaluate_progressive_disclosure>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `explanation` | `ScAddr` of the node whose `nrel_derived_from` chain depth is measured | required | only `IsElement` |
| 2 | `partial_results_group` | `ScAddr` of a container node whose children are counted | required | only `IsElement` |
| 3 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level`:
`1.0` if the provenance chain depth from `explanation` exceeds 4, or
`partial_results_group` has more than 5 children (or both); `0.0`
otherwise. Success — `FinishSuccessfully()` + `FormResult(explanation, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`keynodes/provenance_keynodes.hpp` (`ProvenanceKeynodes::nrel_derived_from`,
reused from [[AgentValidateProvenanceChain]]), `keynodes/adaptive_interface_keynodes.hpp`
(`AdaptiveInterfaceKeynodes` — new file, first agent of this domain,
same minimal-start pattern [[AgentExcludeFromCoalition]] established for
`coalition_keynodes.hpp`).

**Reads:** `nrel_derived_from` arcs transitively reachable from
`explanation`; direct `ConstPermPosArc` children of `partial_results_group`.

**Writes:** only the `value_link` of `result_level` — no violation node,
since the rule text describes no failure mode, only a strategy
selection.

**Workflow.** Data-independent from every other agent in the module.
Reuses `nrel_derived_from`, the same relation [[AgentValidateProvenanceChain]]
walks, but for longest-path depth rather than reachability — the third
agent in the module to perform a multi-hop graph traversal (after
[[AgentValidateProvenanceChain]]'s BFS and [[AgentCheckAcyclicity]]'s
cycle-detecting DFS), and the first to compute a *quantity* (depth)
rather than a boolean (reachable / has-cycle) from one.

## Behavior

```
IF  explanation is not an sc-element
 OR partial_results_group is not an sc-element
 OR result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  depth := ProvenanceDepth(explanation, onStack={}, depthCache={})
  count := number of ConstPermPosArc children of partial_results_group
  needsProgressiveDisclosure := depth > 4 OR count > 5

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (needsProgressiveDisclosure ? 1.0 : 0.0) to value_link
  action.FormResult(explanation, value_link)
  → action.FinishSuccessfully()

ProvenanceDepth(node, onStack, depthCache):
  IF node IN depthCache: RETURN depthCache[node]
  IF node IN onStack: RETURN 0                      // cycle guard
  onStack.insert(node)
  maxDepth := 0
  FOR EACH next WITH node -[derived_from]-> next:
    maxDepth := max(maxDepth, 1 + ProvenanceDepth(next, onStack, depthCache))
  onStack.erase(node)
  depthCache[node] := maxDepth
  RETURN maxDepth
```

**Error handling:** the only path to an error is a missing argument; an
`explanation` with no outgoing `derived_from` (depth 0) and an empty
`partial_results_group` (count 0) is a normal, non-error, `0.0` outcome.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — read-only over
  `nrel_derived_from`/group membership; a pathologically large or deeply
  nested `derived_from` graph could drive the recursive DFS deep, same
  bounded-by-project-scale caveat as [[AgentCheckAcyclicity]].
- No PII involved — the data is structural (provenance chain length,
  result-group size).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(V+E) memoized DFS over the `nrel_derived_from` subgraph reachable from `explanation`, plus O(N) group-membership scan. |
| Accuracy | Not applicable — deterministic threshold check; verified by tests covering each threshold independently, both-below-threshold, and a cyclic `derived_from` fixture proving termination. |
| Cost | Negligible for the chain/group sizes seen in project scenarios. |
| Alerts | None — the indicator is passive; nothing consumes it proactively (no adaptive-interface component exists yet, see Limitations). |

## Tests

File: `tests/test_agent_evaluate_progressive_disclosure.cpp` (5 tests, all pass).

**1. `TriggersOnDeepProvenanceChain`**
Input: a 5-step `derived_from` chain from `explanation`, empty partial-results group.
Output: indicator `1.0` — depth (5) exceeds the threshold (4).

**2. `TriggersOnManyPartialResults`**
Input: a 2-step chain (below threshold), a group with 6 members.
Output: indicator `1.0` — count (6) exceeds the threshold (5).

**3. `DoesNotTriggerWhenBothBelowThreshold`**
Input: a 1-step chain, a group with 3 members.
Output: indicator `0.0`.

**4. `HandlesCycleInDerivedFromWithoutHanging`**
Input: r1 → r2 → r1 (2-cycle) in `derived_from`.
Output: the action completes without hanging (the `onStack` guard is
itself what the test verifies) and finishes successfully.

**5. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
