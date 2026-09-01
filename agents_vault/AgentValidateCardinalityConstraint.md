---
type: agent
tags: [agent, sc-agent, integrity, shacl, validation]
action_class: action_validate_cardinality_constraint
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentValidateCardinalityConstraint

← [[Agent Index]] · from [[Implementation Backlog]] (top-5, #5, last) · implements the core of `rules_integrity_validation.scs`

> [!note] One agent instead of six
> `rules_integrity_validation.scs` formulates six SHACL shapes
> (`shape_user_request_authorship`, `shape_subtask_status_priority`,
> `shape_result_provenance`, `shape_integrated_result_complete`,
> `shape_is_capability`, `shape_context_completeness`). All six
> mechanically reduce to the same action: count the sc-arcs of a
> given relation at the target element and compare the count with
> `[minCount, maxCount]`. Instead of six nearly identical C++ classes,
> one common agent has been implemented, parameterized by the relation
> and bounds; which specific parameter combination implements which
> rule is documented directly in
> `rules/rules_integrity_validation.scs` next to each rule.
> The seventh shape, `shape_provenance_chain_complete`, requires checking
> a transitive path (`nrel_derived_from` to `concept_user_request`),
> not counting arcs — it is not implemented by this agent.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_validate_cardinality_constraint` |
| Type of sc-node of the class | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentValidateCardinalityConstraint : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_validate_cardinality_constraint.{hpp,cpp}` |
| Role | Implements the mechanical core of `agent_integrity_checker` (`rules_integrity_validation.scs`) |
| KB rule | `rules/rules_integrity_validation.scs` → `rule_validate_authorship_shape`, `rule_validate_subtask_status_shape`, `rule_validate_result_provenance_shape`, `rule_validate_integrated_result_shape` (partially), `rule_validate_is_capability_shape` (partially), `rule_validate_context_completeness` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] — same argument about one module per `.dylib`) |

## Purpose

**Problem.** The KB integrity discipline (formulas 4–7,
`concepts/integrity.scs`) requires that every instance of a
certain class has mandatory links — request author, subtask
status, result provenance, etc. Without mechanical verification, such
requirements remain prose in `nrel_sc_text_translation`, not an
executable rule.

**Goal.** Provide a single universal, parameterizable building block
"count the relation arcs at an element and compare with bounds", from
which the project's six SHACL shapes are assembled as special cases
(different calls with different arguments), without duplicating the counting
logic six times.

**Value.** The SHACL compatibility declared in the KB design
(`concepts/integrity.scs`: "Conceptually related to the SHACL approach")
gets a real computable implementation at least for the
`minCount`/`maxCount` part of constraints — the most mechanical and
least ambiguous part of six out of seven shapes.

**Stakeholders:** `agent_integrity_checker` (conceptual "owner" of the
rules in the catalog `concepts/sc_agents.scs`, not implemented as a separate
sc-agent in full — this implementation covers only its cardinality
part), `agent_coalition_reconfigurator` (potential consumer of
`concept_integrity_violation`, not invoked by this agent directly).

## Capabilities

**Capabilities:**
- count sc-arcs of one of three connection patterns for a given
  relation at a given target element:
  - **outgoing non-role** (`target -[ConstCommonArc]-> ?`,
    classified as `relation`) — e.g. `nrel_has_provenance`;
  - **incoming non-role** (`? -[ConstCommonArc]-> target`,
    classified as `relation`) — e.g. `nrel_formulates⁻¹`;
  - **outgoing role** (`target -[ConstPermPosArc]-> ?`,
    classified as `relation`) — e.g. `rrel_goal_context`;
- compare the count with `[minCount, maxCount]` (an empty/non-numeric
  `max_count` link is treated as no upper bound);
- on violation, create an instance of the passed violation class
  (`concept_cardinality_constraint_violation` or
  `concept_existence_constraint_violation` — rules use different
  classes for formally identical counting mechanics), attach it under
  `result_level`, and link it to the checked `target` via
  `nrel_concerns` (`concepts/integrity.scs`, AGENTS_AUDIT.md §6.8);
- finish correctly with an error when any of the seven arguments is
  missing or when `mode` is unrecognized.

**Limitations (by code):**
- **the three connection patterns are not auto-detected**, but an explicitly
  passed `mode` parameter. None of the six shapes formalizes the
  direction/role of the connection structurally — only in prose
  ("property X⁻¹", "property rrel_Y") — therefore three marker constants
  (`concept_cardinality_check_outgoing_non_role` / `_incoming_non_role`
  / `_outgoing_role`) were added together with this implementation
  (`rules_integrity_validation.scs`), rather than taken ready-made from
  the existing ontology.
- **does not check `qualifiedValueShape`** — part of `shape_integrated_result_complete`
  ("no unresolved conflict") and `shape_is_capability` ("target class is
  `concept_capability_profile`") is not implemented: this is not arc
  counting, but a separate check of the target's content/class.
- **`shape_provenance_chain_complete` is not implemented at all** — checking
  the transitive path `nrel_derived_from` is a fundamentally different task
  (graph traversal, not counting incident arcs).
- **the violation's only formal link to `target` is `nrel_concerns`,
  generic on purpose** — the new relation (added together with this fix,
  AGENTS_AUDIT.md §6.8) ranges over `sc_node` on its second domain,
  since the ontology defines no common "any entity" superclass to type
  it more narrowly; it is still attached under `result_level` in
  addition (same technique as `concept_security_incident` in
  [[AgentDetectAnomalousRequestRate]]), not instead.
- **does not distinguish "no arcs at all" from "a related arc exists, but not
  of the right type"** — it simply counts matches by exact relation; if the
  caller passes the wrong relation, the agent will not notice.
- **six shapes = at least six to eight separate calls** (one per property):
  the agent does not accept a list of properties in one call — on one
  hand, simplicity and uniformity of interface; on the other, the
  orchestration of "the whole shape at once" falls on the caller.

## Input

**Trigger:** arc `action_initiated -> <instance of action_validate_cardinality_constraint>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `target` | existing sc-element | required | only `IsElement` |
| 2 | `relation` | existing sc-relation (`nrel_*`/`rrel_*`) | required | only `IsElement` |
| 3 | `mode` | one of `concept_cardinality_check_outgoing_non_role` / `_incoming_non_role` / `_outgoing_role` | required | `IsElement` + check against three known constants; otherwise → error |
| 4 | `min_count` | numeric sc-link | required | `IsElement`; must parse as a number, otherwise → error |
| 5 | `max_count` | numeric sc-link | required as an element, but allows empty/non-numeric content | `IsElement`; unparsable content is treated as "no upper bound", not an error |
| 6 | `violation_class` | existing violation `sc_node_class` | required | only `IsElement` |
| 7 | `result_level` | indicator receiver node | required | only `IsElement` |

## Output

A new/updated sc-link `value_link` on `result_level` with `1.0`
(within bounds) or `0.0` (violation). If violation — additionally:
a new instance of `violation_class`, attached under `result_level`
via `ConstPermPosArc` and linked to `target` via
`violation -[nrel_concerns]-> target`. Success —
`FinishSuccessfully()` + `FormResult(target, valueLink)`.

## Connections

**Dependencies:** `common/float_link_io.hpp` (`ReadFloatLinkContent`/
`WriteFloatLinkContent` — reused unchanged, third time in a row),
`common/relation_io.hpp` (`LinkByRelation` — for `nrel_concerns`),
`keynodes/integrity_keynodes.hpp` (`IntegrityKeynodes`, extended with
`nrel_concerns`).

**Reads:** arcs of `relation` at `target` (in one of the three patterns),
content of `min_count`/`max_count`.

**Writes:** `value_link` to `result_level`; on violation — a new node of
class `violation_class`, plus a `nrel_concerns` link from it to `target`.

**Workflow.** Independent of the other nine agents of the project in terms
of data — does not read or write trust/provenance/subtasks/integration.
The only agent operating in the "KB integrity" domain
(`concept_integrity_constraint`/`concept_integrity_violation`).

## Behavior

```
IF any of the seven arguments is not an sc-element
THEN FinishWithError()

ELSE IF mode is not one of the three known markers
THEN FinishWithError()

ELSE IF min_count does not parse as a number
THEN FinishWithError()

ELSE
  hasMax := max_count parses as a number
  count  := number of relation arcs at target according to mode pattern

  valid := count >= minCount AND (NOT hasMax OR count <= maxCount)

  IF NOT valid
  THEN
    v := create a node of class violation_class
    attach v under result_level (ConstPermPosArc)
    link v -[nrel_concerns]-> target

  write (valid ? 1.0 : 0.0) to value_link result_level (create if missing)
  action.FormResult(target, value_link)
  → action.FinishSuccessfully()
```

**Error handling/timeouts:** same as other agents — `DoProgram` is
synchronous, waiting is on the caller side via `InitiateAndWait`.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the agent trusts the caller in choosing
  `relation`/`mode`/`violation_class` — it can be invoked with a mutually
  inconsistent triple (for example, a role relation in `outgoing_non_role`
  mode) and yield a count of 0 with no error signal, which will look like
  a justified violation, but is in fact a caller error.
- It is not checked that `target` actually belongs to the class
  assumed by the given shape (`targetClass` from the rule prose) —
  the agent will check the cardinality of the relation at any passed node.
- Data (arc counters, violation classes) is not PII.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; linear in the number of incident arcs of the given relation at `target` (usually single digits). |
| Accuracy | Not applicable — deterministic comparison of count with bounds; correctness verified by tests on both patterns (non-role outgoing/incoming) and both outcomes (within bounds/violation). |
| Cost | Negligible. |
| Alerts | Not implemented. An alert on a series of violations of the same `violation_class` in a short period would be appropriate — a signal of a systematic data generation problem, not an isolated case. |

## Tests

File: `tests/test_agent_validate_cardinality_constraint.cpp` (3 tests,
all pass; all three scenarios were additionally verified on a live
sc-server with real ontology data).

**1. `ValidatesWhenCountWithinBounds`**
Reproduces `shape_result_provenance`: three outgoing links of one
relation, `minCount=1`, no `maxCount`.
Output: indicator `1.0`, no violation created.
On the live server — `shape_context_completeness` on the real
`interaction_context_001` (`rrel_goal_context`, from
`scenario_request_processing.scs`, where all 5 subcontexts are specified):
indicator `1.0`, no violation.

**2. `CreatesViolationWhenCountBelowMinimum`**
Reproduces `shape_user_request_authorship`: `nrel_formulates⁻¹` with no
incoming user at all, `minCount=1, maxCount=1`.
Output: indicator `0.0`, a node of class
`concept_cardinality_constraint_violation` created and attached under
`result_level`, linked to `target` via `nrel_concerns`.
On the live server — the same shape on the real `request_001`
(in the KB there is exactly one `user_alice => nrel_formulates: request_001`):
indicator `1.0` (within bounds `[1,1]`), no violation; the violating case
was separately tested on an isolated node without a single
`nrel_generated_in_context` (`shape_result_provenance`) — indicator
`0.0`, violation of class `concept_existence_constraint_violation`
created correctly.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
