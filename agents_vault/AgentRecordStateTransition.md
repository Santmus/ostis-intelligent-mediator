---
type: agent
tags: [agent, sc-agent, lifecycle, orchestration, provenance]
action_class: action_record_state_transition
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentRecordStateTransition

← [[Agent Index]] · Phase 1 of the lifecycle-orchestrator gap
(AGENTS_AUDIT.md §7.1), added 2026-09-03 after a plan session that
compared three equally-sized missing subsystems (orchestrator, external
system bridge, ontology translation service) and picked the orchestrator
as best-grounded — its ontology already existed in full and was read by
zero of the 26 agents that came before it.

> [!note] Mechanical executor, not a deciding orchestrator
> This agent does not decide *when* or *to which state* a request
> transitions — the caller supplies `to_state_class`. No agent in this
> module calls another agent's action (composition is always external,
> see `test_integration_request_pipeline.cpp`), and no formula in the
> text says when e.g. `x_verify → x_present` should fire — exactly the
> class of open-ended gap this project has consistently declined to
> invent. What this agent *does* own: checking the transition is on the
> admissibility graph (`nrel_transitions_to`), enforcing the one
> resource rule the text actually states
> (`constraint_separate_resource_decrease`), and building the complete
> transition-provenance bundle (`constraint_transition_provenance`) so
> the fact of the transition is never recorded half-built.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_record_state_transition` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentRecordStateTransition : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_record_state_transition.{hpp,cpp}` |
| Role | Operationalizes `constraint_transition_provenance` / `constraint_separate_resource_decrease` (`axioms/axioms_constraints.scs`) directly — the first agent in the module to implement a `constraint_*`/`affirmation_*` rather than a `rule_*` text |
| Knowledge base declaration | `rules/rules_mediation_orchestration.scs` (new file — `action_record_state_transition <- sc_node_class`) |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** The full lifecycle model (`concepts/mediation_lifecycle.scs`)
— 13 states, the `nrel_transitions_to` admissibility graph, reified
`concept_state_transition` with its four role relations, plus the
budget/depth resources `b_q`/`d_max` (`concepts/processing_plan.scs`) —
existed in the KB from before this implementation pass began, completely
unused: no agent read `nrel_has_state`, none ever built a
`concept_state_transition` node, and no request in any test or scenario
had its state advanced anywhere except as static fixture text
(`scenario_lifecycle_recovery.scs`).

**Goal.** Given a request already sitting at some state instance and a
caller-decided target class, verify the edge is admissible and
resource-legal, then atomically produce everything
`constraint_transition_provenance` requires: a new (immutable) state
instance, the reified transition node, and its audit record — or refuse
and change nothing.

**Value.** The first agent to touch `mediation_lifecycle.scs` at all.
Scoped deliberately to the slice of the pipeline that already has real
agents on both sides of it (dispatch/register/propagate on the
`x_exec` side, resolve/validate-integration on the `x_aggregate` side)
so its correctness is demonstrable end-to-end today, rather than
speculatively against states nothing produces yet.

**Stakeholders:** any future caller that needs to advance a request's
state — currently only test code and, eventually, whatever Phase 2
builds for the nine still-missing §7.5 agents and the replanning cycle.

## Competence

**Capabilities:**
- resolves the class of the request's current state instance among the
  four classes in scope (`concept_state_execution`/`_assignment`/
  `_aggregation`/`_verification`) by checking class membership, not by
  trusting a caller-supplied "from" class;
- refuses if that class isn't one of the four (state outside Phase 1's
  scope) or if the (from, to) edge has no `nrel_transitions_to` arc;
- on `x_exec → x_assign` specifically: refuses if
  `nrel_remaining_reassignment_budget` is already `0`, otherwise
  decrements it by exactly `1`; every other edge carries both
  `nrel_remaining_reassignment_budget` and
  `nrel_remaining_replanning_depth` forward unchanged onto the new state
  instance (states are immutable snapshots — confirmed against
  `scenario_lifecycle_recovery.scs`'s own fixture shape, never mutated
  in place);
- rewrites `request`'s `nrel_has_state` to the new instance
  (`SetFunctionalRelationTarget` — the old arc is erased, not left
  dangling);
- builds the full `constraint_transition_provenance` bundle: the
  transition node (`concept_state_transition`, `rrel_from_state`/
  `rrel_to_state`/`rrel_transition_event`), `nrel_caused_by_event`,
  `nrel_uses_plan_version`, `nrel_transition_recorded_in` pointing at a
  fresh `concept_audit_record` + `concept_audit_record_transition`,
  itself given an `nrel_has_provenance` record — matching
  [[AgentRegisterPartialResult]]'s precedent of never leaving an audit
  record without one, even though the hard constraint text only lists
  five facts and doesn't name provenance-on-the-audit-record.

**Limitations (per code):**
- **does not decide when to transition or which class to target** — see
  the note above; a `to_state_class` the caller shouldn't have chosen
  yet is not detected, only classes with no admissibility edge are;
- **does not re-verify the real-world precondition of the transition**
  — e.g. that every subtask in `T(q)` is actually `completed` before
  `x_exec → x_aggregate` — the same "trust the caller" stance
  [[AgentReassignOnResourceExhaustion]] takes for `resource_exhausted`;
- **only four state classes recognized** — `concept_state_accepted`,
  `_interpretation`, `_planning`, `_replanning`, `_presentation`,
  `_partial`, `_clarification`, `_refusal`, `_cancelled` are Phase 2;
- **`rrel_transition_actor`/`rrel_context` never written** — not part
  of the hard constraint's five-fact minimum; deferred as optional
  arguments that can be added without touching the code already
  written, per the approved plan;
- **does not distinguish reassignment from replanning beyond the one
  edge it knows** — `nrel_remaining_replanning_depth`/`d_max` is
  carried but never decremented; the `x_assign → x_plan` replanning
  edge is Phase 2;
- only `IsElement` on all four arguments, as with the rest of the
  module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_record_state_transition>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `request` | `ScAddr` of a `concept_user_request` whose current `nrel_has_state` is read and rewritten | required | `IsElement`; must already have `nrel_has_state` pointing at a state instance of one of the four recognized classes |
| 2 | `to_state_class` | `ScAddr` of the target `concept_state_*` | required | `IsElement`; admissibility checked via `nrel_transitions_to` against the resolved "from" class |
| 3 | `transition_event` | `ScAddr` of an already-typed `concept_transition_event` subclass instance (e.g. `concept_event_reassign`) | required | only `IsElement` — typing is the caller's responsibility |
| 4 | `plan_version` | `ScAddr` of the request's current `concept_plan_version` | required | only `IsElement` |

## Output

Success — `FinishSuccessfully()` + `FormResult(new_state, transition,
audit_record)`. Failure at any check (missing argument, no current
state, unrecognized "from" class, inadmissible edge, exhausted budget on
the reassignment edge) — `FinishWithError()`, and nothing is created:
`request`'s `nrel_has_state` still points at the original instance.

## Links

**Dependencies:** `common/relation_io.hpp` (`TargetOf`, `LinkByRelation`,
`LinkByRole`, `SetFunctionalRelationTarget` — all reused unchanged, no
new shared helper needed), `common/float_link_io.hpp`
(`ReadFloatLinkContent`/`WriteFloatLinkContent`), new
`keynodes/lifecycle_keynodes.hpp` (`LifecycleKeynodes` — every element
it declares already existed in the KB; the file only collects references
to them, the same pattern [[AgentLinkExplanationToProvenance]] used for
`ProvenanceKeynodes`).

**Reads:** `request`'s `nrel_has_state`; the current state instance's
`nrel_remaining_reassignment_budget`/`nrel_remaining_replanning_depth`;
the class-level `nrel_transitions_to` graph between `concept_state_*`
nodes.

**Writes:** a new state instance (classified `to_state_class`, two fresh
resource value-links); `request`'s `nrel_has_state` (old arc erased);
a `concept_state_transition` node with three role arcs and three
functional relations; a `concept_audit_record`/`_transition` node with
its own provenance record.

**Workflow.** Sits between whatever produced the request's current state
and whatever needs the new one. In the extended integration test, it
brackets [[AgentValidateIntegration]]: `x_exec → x_aggregate` runs after
[[AgentDispatchReadySubtask]]/[[AgentRegisterPartialResult]] complete
the last subtask, and `x_aggregate → x_verify` runs after
[[AgentValidateIntegration]] confirms the integrated result is
explained and conflict-free — three unrelated agent families chained by
shared request/result data, not by this agent calling any of them.

## Behavior

```
IF  request is not an sc-element
 OR to_state_class is not an sc-element
 OR transition_event is not an sc-element
 OR plan_version is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

old_state := TargetOf(request, nrel_has_state)
IF old_state is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

from_class := the one of {x_exec, x_assign, x_aggregate, x_verify}
              old_state is classified by, else Empty
IF from_class is Empty
THEN  log error → action.FinishWithError()  [STOP]

IF NOT nrel_transitions_to(from_class, to_state_class)
THEN  log error → action.FinishWithError()  [STOP]

b_rem := old_state.nrel_remaining_reassignment_budget  (must exist)
d_rem := old_state.nrel_remaining_replanning_depth       (must exist)
IF either link is missing
THEN  log error → action.FinishWithError()  [STOP]

IF from_class == x_exec AND to_state_class == x_assign:
  IF b_rem <= 0  THEN  log error → action.FinishWithError()  [STOP]
  b_rem := b_rem - 1
# else: b_rem, d_rem carried over unchanged

new_state := fresh node <- to_state_class
new_state.nrel_remaining_reassignment_budget := b_rem
new_state.nrel_remaining_replanning_depth    := d_rem
SetFunctionalRelationTarget(request, nrel_has_state, new_state)

transition := fresh node <- concept_state_transition
LinkByRole(transition, old_state,        rrel_from_state)
LinkByRole(transition, new_state,        rrel_to_state)
LinkByRole(transition, transition_event, rrel_transition_event)
LinkByRelation(transition, transition_event, nrel_caused_by_event)
LinkByRelation(transition, plan_version,     nrel_uses_plan_version)

audit := fresh node <- concept_audit_record, concept_audit_record_transition
LinkByRelation(transition, audit, nrel_transition_recorded_in)
provenance := fresh node <- concept_provenance_record
LinkByRelation(audit, provenance, nrel_has_provenance)

action.FormResult(new_state, transition, audit)
→ action.FinishSuccessfully()
```

**Error handling:** every failure path is checked before any node is
created — a partially-built transition can never be observed.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** a caller can pass any `transition_event`/
  `plan_version` it likes — the agent does not verify either represents
  what actually happened, only that both are sc-elements. This mirrors
  every other agent's "trust the caller for typing" stance, but here the
  consequence (a falsified transition-provenance record) is closer to
  the audit trail's own integrity than most agents touch.
- No PII involved — the data is structural (state/event/plan identity).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — four fixed candidate classes, one admissibility lookup, two resource reads. |
| Accuracy | Not applicable — deterministic; verified by tests covering both non-resource edges, the resource-decrementing edge, budget exhaustion, and an inadmissible edge. |
| Cost | Negligible; two extra sc-nodes (transition + audit record) plus two value-links per call versus a hypothetical no-provenance version. |
| Alerts | None. |

## Tests

File: `tests/test_agent_record_state_transition.cpp` (6 tests, all pass).
Also exercised end-to-end in
`tests/test_integration_request_pipeline.cpp`'s
`RecordsStateTransitionsAroundIntegrationValidation`.

**1. `TransitionsExecutionToAggregationCarryingResourcesUnchanged`**
Input: state at `x_exec`, `b_rem=3`, `d_rem=2`, admissible edge to
`x_aggregate`.
Output: new state at `x_aggregate` with both resources unchanged;
full transition bundle (roles, `nrel_caused_by_event`,
`nrel_uses_plan_version`, audit record + its provenance record) verified
directly.

**2. `TransitionsAggregationToVerificationCarryingResourcesUnchanged`**
Same non-resource shape, on the other edge covered by Phase 1.

**3. `TransitionsExecutionToAssignmentDecrementingReassignmentBudget`**
Input: `x_exec → x_assign`, `b_rem=2`.
Output: new state's `b_rem=1`, `d_rem` unchanged.

**4. `FinishesWithErrorWhenReassignmentBudgetExhausted`**
Input: same edge, `b_rem=0`.
Output: `FinishedWithError`; `request.nrel_has_state` still points at
the original state instance.

**5. `FinishesWithErrorOnInadmissibleTransition`**
Input: `x_exec → x_verify` directly, no `nrel_transitions_to` edge
declared for the fixture.
Output: `FinishedWithError`; no mutation.

**6. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
