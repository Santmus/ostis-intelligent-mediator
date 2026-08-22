---
type: agent
tags: [agent, sc-agent, provenance, ratio]
action_class: action_compute_pi_metric
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentComputePiMetric

← [[Agent Index]] · from [[Implementation Backlog]] (top-5, #1) · first agent outside [[Trust Model|trust model]]

> [!note] One agent — three metrics
> The rule `rule_compute_pi_metrics` describes π_proc, π_dec, π_data as
> **the same calculation** "share of elements with provenance" over three
> different sets. The ontology does not provide a single formal class for
> Proc/Dec/Data (unlike `concept_trust_level`, which is common to all
> trust components), so the agent is implemented in a generalized way: **the caller**
> determines which specific metric to compute by the group of
> elements it passes.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_compute_pi_metric` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentComputePiMetric : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_compute_pi_metric.{hpp,cpp}` |
| Role | Implements the computable part of rule `rule_compute_pi_metrics` (the `agent_provenance_registrar` part from directory `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_provenance_completion.scs` → `rule_compute_pi_metrics` |
| Module | `TrustModule` (not renamed — sc-machine allows one module per shared library; see comment in `trust_module.cpp`) |

## Purpose

**Problem.** The transparency model π = ⟨π_proc, π_dec, π_data⟩ (see
`concepts/provenance.scs`) must be able to answer the question "what share
of processes/decisions/used data is actually supplied with a provenance
record?" — without this, claims about the traceability of collective
processing remain declarative.

**Goal.** For an explicitly passed set of elements, calculate the share of
those that have an outgoing `nrel_has_provenance` relation to
`concept_provenance_record`.

**Value.** A single, reusable calculation for all three transparency
metrics — no need to write three separate agents with almost identical
code; the agent is genuinely neutral about WHAT exactly is being measured.

**Stakeholders:** `agent_integrity_checker` (the metric is an integrity
indicator), auditors/regulators, developers tracking the completeness of
the provenance subsystem.

## Competence

**Capabilities:**
- compute `|group|` (all elements that the group points to via
  `ConstPermPosArc`) and `|{x ∈ group : has_provenance(x)}|`;
- compute the ratio `pi = withProvenance / total`;
- write the result into a numeric sc-link of the target node, creating it on
  first access (same pattern as [[AgentUpdateTrustReliability|trust agents]]);
- finish with an error correctly when one of the two arguments is missing.

**Limitations (per code):**
- **does not implement** the part of the rule "if π < 1.0 — create a
  `concept_integrity_violation` of type `transparency_gap`; run
  `rule_fill_missing_provenance`" — the ontology has no formally
  defined class/relation for typing a violation specifically as a
  `transparency_gap` (only free-form rule text), and
  `rule_fill_missing_provenance` itself is a heuristic without an algorithm
  (reconstruction from event logs/history/timestamps is not
  formalized). The agent **only computes the number** and takes no
  action based on it.
- **an empty group yields `pi = 0`, not an error** — mathematically 0/0 is
  undefined, but the agent treats "nothing to measure" as "zero
  completeness" rather than an exceptional situation. If the caller
  accidentally passes an empty/incorrect group, it will be indistinguishable
  from "total provenance failure".
- **does not distinguish Proc/Dec/Data** — the correctness of the result
  entirely depends on what the caller placed in the group; if an element of
  the wrong type ends up there (e.g., a decision instead of a process),
  the agent will not notice.
- as with trust agents — does not verify that the arguments are
  semantically meaningful in general (only `IsElement`).
- an element is considered to "have provenance" if there is **any**
  outgoing `nrel_has_provenance` relation to anything, without checking that
  the target is actually a `concept_provenance_record`.

## Input

**Trigger:** arc `action_initiated -> <instance of action_compute_pi_metric>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `elements_group` | `ScAddr` of a node with outgoing `ConstPermPosArc`s to the elements being measured | required | only `IsElement` |
| 2 | `metric_level` | `ScAddr` of the metric target node | required | only `IsElement` |

**Format:** `elements_group` is not an sc-link or a number, but an ordinary node
acting as a set container (membership is simple `ConstPermPosArc`
arcs to elements of any type).

## Output

The updated/created numeric sc-link `value_link` on `metric_level`
with content `std::to_string(pi)`. Side effect — generation of
`value_link` + a connecting arc on first call. Success —
`FinishSuccessfully()` + `FormResult(metricLevel, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (only `WriteFloatLinkContent`
— there are no reads through it, values are read directly by the sign of
relation existence, not as a number), `keynodes/provenance_keynodes.hpp`
(`ProvenanceKeynodes::nrel_has_provenance`, `action_compute_pi_metric`).

**Reads:** membership of `elements_group` (`ConstPermPosArc`), presence of
`nrel_has_provenance` for each member.

**Writes:** the `value_link` of node `metric_level`.

**Workflow.** The first agent in the project outside the trust vector
— the beginning of `agent_provenance_registrar` implementation. Not formally
linked to [[AgentAggregateTrust]] and the others, but conceptually both
families (trust and provenance) feed the integrity of the ecosystem,
which the still-unimplemented `agent_integrity_checker` should verify (see
[[Implementation Backlog]]).

## Behavior

```
IF  elements_group is not an sc-element  OR  metric_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  total := 0; withProvenance := 0
  FOR EACH member IN elements_group (via outgoing ConstPermPosArc):
    total += 1
    IF  ∃ outgoing ConstCommonArc from member marked nrel_has_provenance
    THEN  withProvenance += 1

  pi := total > 0 ? withProvenance / total : 0.0   // empty group -> 0, not error

  IF  metric_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc metric_level -> value_link

  write pi into value_link
  action.FormResult(metric_level, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is the absence of one of the
two argument nodes; absence of `nrel_has_provenance` relations for group
members or an empty group are not considered errors (see Limitations).

**Timeouts:** as with the rest of the module's agents — there is no internal
timeout, `DoProgram` is synchronous; waiting is on the caller side
via `InitiateAndWait`.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full
  access to sc-memory within the `sc-server` process.
- **Specific risk:** because the criterion "has provenance" is the
  presence of *any* `nrel_has_provenance` relation without checking the target,
  a node can be "falsely" marked as having provenance by linking it to
  an arbitrary (not necessarily `concept_provenance_record`) element
  — the π metric will be artificially inflated without real auditing.
- The π metric is an abstract number and contains no PII.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; linear in group size — O(N) iterators (one `CreateIterator5` per element) + O(1) to write the result. For large groups (thousands of elements), significantly more expensive than 2-argument trust agents. |
| Accuracy | Not applicable to the formula itself (deterministic division); correctness is verified by tests, including a reproduction of the reference scenario. |
| Cost | Negligible for typical group sizes. |
| Alerts | Not implemented — same as other agents in the project. An alert for "π dropped sharply between periodic computations" would be especially appropriate (in the spirit of the original rule "if π_• < 1.0 create a violation"), but this part of the rule was deliberately not implemented (see Limitations). |

## Tests

File: `tests/test_agent_compute_pi_metric.cpp` (3 tests, all pass; scenario 2 additionally verified on a live sc-server — result `0.600000`).

**1. `ReproducesScenarioAllFiveHaveProvenance`**
Input: group of 5 elements, all have `nrel_has_provenance`
(reproduces "π_dec = 5/5 = 1.0" from `scenario_explanation_construction.scs`).
Output: `pi = 1.0`, `FinishedSuccessfully`.

**2. `ComputesPartialCoverageRatio`**
Input: group of 5 elements, 3 have provenance, 2 do not.
Output: `pi = 3/5 = 0.6`, `FinishedSuccessfully` — confirms that
the computation is non-trivial (not always 0 or 1).

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
