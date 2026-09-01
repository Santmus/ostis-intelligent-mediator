---
type: agent
tags: [agent, sc-agent, cooperation, state-machine]
action_class: action_propagate_to_dependent
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentPropagateToDependent

← [[Agent Index]] · from [[Implementation Backlog]] (`rules_cooperation.scs`, after [[AgentRegisterPartialResult]])

> [!note] The subtask is never the addressee of the communicative act (AGENTS_AUDIT.md 6.10)
> An earlier version of this agent made σ' itself the `rrel_receiver` of
> the inform act, even though a subtask is not a participant in
> communication. The fix: the semantic fact "σ' now has r as an input" is
> recorded unconditionally via `nrel_is_input_for(r, σ')`
> (`relations/relations_execution.scs`), independent of whether σ' has an
> assignee yet. The communicative act is created **in addition**, and
> only when σ' already has `nrel_assigned_to` — its receiver is that
> assigned system, never σ' itself. Before `task_allocator` has run there
> is no communication participant to address, so `is_input_for` alone is
> enough for the result to count as propagated.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_propagate_to_dependent` |
| sc-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentPropagateToDependent : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_propagate_to_dependent.{hpp,cpp}` |
| Role | Implements `rule_propagate_to_dependent` (`agent_cooperative_executor`, `rules_cooperation.scs`) |
| KB rule | `rules/rules_cooperation.scs` → `rule_propagate_to_dependent` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] — same argument about one module per `.dylib`) |

## Purpose

**Problem.** Completion of subtask σ by itself does not make its result
visible to subtasks σ' that depend on it
(`nrel_subtask_depends_on`) — without an explicit step, they would have
to be polled or rely on an external orchestrator.

**Goal.** Find all σ' that depend on the just-completed σ and make the
result r an input of each of them via `nrel_is_input_for(r, σ')`; for
every σ' that already has an assigned executor, additionally notify that
executor with a communicative act `concept_knowledge_exchange_act` of
type `concept_act_inform`.

**Value.** Closes the subtask dependency graph
(`nrel_subtask_depends_on`) into an executable push mechanism: as soon as
σ is completed, the result becomes formally available as input to every
σ' waiting for it, and any already-assigned executor learns about it
without polling — the same principle demonstrated by `inform` act
exchange in `scenario_knowledge_exchange.scs`.

**Stakeholders:** `agent_cooperative_executor` (conceptual “owner” of the
rule, fixed sender of the act), subtasks σ' waiting for input from σ.

## Competence

**Capabilities:**
- verify that σ is indeed `completed` (the agent is safe for
  speculative invocation — same as [[AgentDispatchReadySubtask]]);
- find all σ' with an outgoing edge `σ' -[nrel_subtask_depends_on]-> σ`
  (reverse traversal — not the same order as read by
  `AllDependenciesCompleted` in [[AgentDispatchReadySubtask]]);
- for each found σ' link `r -[nrel_is_input_for]-> σ'`
  unconditionally (`relations/relations_execution.scs`);
- additionally, when σ' already has `nrel_assigned_to`, generate a
  `concept_knowledge_exchange_act` with sender=
  `agent_cooperative_executor`, receiver=that assigned system (not σ'),
  act_type=`concept_act_inform`, payload=`r`;
- write into the receiver node the number of dependent σ' found (0 if σ
  is not yet `completed` or has no dependent σ'; this counts
  `is_input_for` links, not communicative acts), creating an sc-link on
  first access;
- terminate correctly with an error when mandatory arguments are missing.

**Limitations (per code):**
- **does not fill** `rrel_context`/`rrel_timestamp` in the created acts
  — same gap as [[AgentDispatchReadySubtask]]: the rule does not specify
  a context/time source.
- **sender of the act is fixed `agent_cooperative_executor`**,
  not a parameter — same limitation as [[AgentDispatchReadySubtask]].
- **no communicative act for an unassigned dependent** — `is_input_for`
  is still set, but if `task_allocator` has not yet assigned an executor
  to σ', no inform act is created for it; a later assignment does not
  retroactively trigger one (this agent is not re-invoked by
  `task_allocator`).
- **does not check cyclicity** of the dependency graph — same as
  [[AgentDispatchReadySubtask]], cycle detection is outside the scope of
  this agent (`rule_check_acyclicity`, not implemented).
- **indicator is not boolean but a counter** of dependent σ' found — the
  only agent in the project where the numeric indicator is not 0/1 but
  an arbitrary non-negative number; it does not distinguish "σ' got only
  `is_input_for`" from "σ' also got an inform act".

## Input

**Trigger:** edge `action_initiated -> <instance of action_propagate_to_dependent>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `result` | partial result r obtained from σ | required | only `IsElement` |
| 2 | `subtask` | just-completed subtask σ | required | only `IsElement` |
| 3 | `result_level` | indicator receiver node | required | only `IsElement` |

## Output

Updated/created sc-link `value_link` on `result_level` with value equal
to the number of dependent subtasks found (0 if σ is not yet
`completed`). For each found σ' — `result -[nrel_is_input_for]-> σ'` is
always created; additionally, if σ' has `nrel_assigned_to`, one new
`concept_knowledge_exchange_act` (sender=`agent_cooperative_executor`,
receiver=the assigned system, act_type=`concept_act_inform`,
payload=`result`). Success — `FinishSuccessfully()` +
`FormResult(subtask, valueLink)`.

## Connections

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`LinkByRelation` — for `is_input_for`;
`TargetOf` — to read σ''s `nrel_assigned_to`; `LinkByRole` — moved here
together with [[AgentRegisterPartialResult]], generalization of the
local `LinkByRole` from [[AgentDispatchReadySubtask]]),
`keynodes/task_keynodes.hpp` (`TaskKeynodes`, including the new
`nrel_is_input_for` keynode).

**Reads:** `nrel_subtask_status` of subtask σ, all σ' with an outgoing
`nrel_subtask_depends_on` to σ (reverse traversal), `nrel_assigned_to`
of each found σ'.

**Writes:** one `nrel_is_input_for` link for each found σ', one
`concept_knowledge_exchange_act` for each σ' that already has an
assignee, `value_link` `result_level`.

**Workflow.** Third and final step in the lifecycle of a subtask in
`agent_cooperative_executor`: [[AgentDispatchReadySubtask]] →
[[AgentRegisterPartialResult]] (sets σ to `completed`, which is read here
as a precondition) → **AgentPropagateToDependent**.

## Behavior

```
IF  result is not an sc-element  OR  subtask is not an sc-element  OR  result_level is not an sc-element
THEN  FinishWithError()

ELSE
  status     := TargetOf(subtask, nrel_subtask_status)
  completed  := (status == concept_subtask_status_completed)
  count      := 0

  IF  completed
  THEN
    FOR EACH σ' WITH σ' -[nrel_subtask_depends_on]-> subtask:
      link result -[nrel_is_input_for]-> σ'
      assignee := TargetOf(σ', nrel_assigned_to)
      IF assignee is an sc-element
      THEN
        create concept_knowledge_exchange_act:
          rrel_sender   = agent_cooperative_executor
          rrel_receiver = assignee
          rrel_act_type = concept_act_inform
          rrel_payload  = result
      count += 1

  write count into value_link result_level (create if absent)
  action.FormResult(subtask, value_link)
  → action.FinishSuccessfully()
```

**Error handling / timeouts:** same as the other agents — `DoProgram`
is synchronous; waiting is on the caller side via `InitiateAndWait`.

## Security

- Privileges — same as the other agents of `TrustModule`: no ACL, full
  access to sc-memory within the `sc-server` process.
- **Specific risk:** the agent marks an arbitrary `result` as input for
  ANY subtask found as dependent on the passed `subtask`, and sends a
  communicative act to its assigned executor, without verifying that the
  caller was authorized to do so — same project-wide absence of action
  initiator authentication, here with a real side effect (one
  `is_input_for` link plus, where applicable, one communicative act per
  σ'). Addressing the actual assignee rather than the subtask node
  narrows the blast radius compared to the earlier version, but does not
  add authorization.
- It is not verified that `result`/`subtask` actually belong to the
  corresponding ontology classes (only `IsElement`).
- Data — not PII directly, but `concept_knowledge_exchange_act` carries
  `rrel_payload=result`, which may contain a real user result.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; linear in the number of direct dependent subtasks (usually small). |
| Accuracy | Not applicable — deterministic graph traversal, not prediction; correctness is verified by tests on both outcomes (completed / not completed). |
| Cost | Negligible. |
| Alerts | Not implemented. |

## Tests

File: `tests/test_agent_propagate_to_dependent.cpp` (4 tests, all pass). Also covered by the second `TEST_F` in `tests/test_integration_request_pipeline.cpp` (`DispatchesRegistersAndPropagatesAlongSubtaskChain`), where it acts as the final link in the Dispatch → Register → Propagate chain, and where the dependent subtask's assignee (not the subtask itself) is asserted as the inform act's receiver.

**1. `PropagatesToAllDependentsWhenSubtaskCompleted`**
Input: σ is `completed`, two dependent σ' (`σ' -[depends_on]-> σ`), each
already assigned to its own executor.
Output: indicator `2.0`; `result -[nrel_is_input_for]-> σ'` set for both;
an inform act created for both, with `rrel_receiver` = the assigned
executor (not σ') and `rrel_payload=result`; neither σ' is ever a
`rrel_receiver` of anything.

**2. `MarksInputForUnassignedDependentWithoutCommunicationAct`**
Input: σ is `completed`, one dependent σ' with no assigned executor.
Output: indicator `1.0`; `result -[nrel_is_input_for]-> σ'` is still set;
no communicative act is created, and σ' is never a `rrel_receiver`.

**3. `DoesNotPropagateWhenSubtaskNotCompleted`**
Input: σ in `in_progress` status, one dependent σ'.
Output: indicator `0.0`, no `is_input_for` link and no act created.

**4. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
