---
type: agent
tags: [agent, sc-agent, cooperation, provenance]
action_class: action_register_partial_result
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentRegisterPartialResult

← [[Agent Index]] · from [[Implementation Backlog]] (`rules_cooperation.scs`, after [[AgentDispatchReadySubtask]])

> [!note] Gated, not unconditional (AGENTS_AUDIT.md 6.2)
> An earlier version of this agent recorded the result and completed the
> subtask unconditionally, without checking that the caller was actually
> the assigned executor or that the subtask was in a state where
> completion makes sense. It now requires both `system ==
> nrel_assigned_to(subtask)` and `nrel_subtask_status(subtask) ==
> in_progress` before writing anything — either failing gate is
> `FinishWithError()`, and the subtask's status is left untouched. Unlike
> [[AgentDispatchReadySubtask]] and [[AgentPropagateToDependent]], there
> is still no numeric readiness indicator: success either records the
> full provenance/execution/audit chain and completes the subtask, or
> nothing happens at all.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_register_partial_result` |
| sc-node type of the class | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentRegisterPartialResult : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_register_partial_result.{hpp,cpp}` |
| Role | Implements `rule_register_partial_result` (`agent_cooperative_executor`, `rules_cooperation.scs`) |
| KB rule | `rules/rules_cooperation.scs` → `rule_register_partial_result` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] — same argument about one module per `.dylib`) |

## Purpose

**Problem.** The executor system sends a partial result for a subtask, but without explicitly recording this fact in sc-memory the result remains "unaccounted for": unclassified, not linked to the system that produced it, the subtask that generated it, or the interaction context — and the subtask itself is still formally considered incomplete.

**Goal.** After verifying that the sending system is the subtask's
assigned executor and the subtask is `in_progress`, atomically record
the provenance triplet `produces`/`caused_by_subtask`/`generated_in_context`,
classify the result as both `concept_partial_result` and
`concept_processing_result`, reify the fact of execution as an
`concept_execution_act` with an accompanying `concept_audit_record`, add
a minimal `concept_provenance_record` to both the result and the audit
record, and only then transition the subtask to `completed` — in a
single, deterministic step.

**Value.** Implements the same logic described in `axioms/axioms_provenance.scs` (the automatic instantiation template when executing a subtask) and reproduced as a fixture in `scenario_request_processing.scs` (step 5, `result_routes`); the execution-act/audit-record/provenance chain closes the gap flagged in AGENTS_AUDIT.md §6.2, where a result could be registered and a subtask completed with no reified record of who did it or when.

**Stakeholders:** `agent_cooperative_executor` (conceptual "owner" of the rule), [[AgentPropagateToDependent]] (next step in the same subtask lifecycle — reads the `completed` status set here), `agent_provenance_registrar` (augments the same result with PROV-O links `wasGeneratedBy`/`wasAttributedTo` — outside this agent's scope).

## Competence

**Capabilities:**
- verify `system == TargetOf(subtask, nrel_assigned_to)` — reject
  otherwise;
- verify `TargetOf(subtask, nrel_subtask_status) ==
  concept_subtask_status_in_progress` — reject otherwise;
- classify the passed result node as both `concept_partial_result` and
  `concept_processing_result`;
- create three non-role binary relations: `produces(s, r)`, `caused_by_subtask(r, σ)`, `generated_in_context(r, C)`;
- create an `concept_execution_act` (also classified
  `concept_execution_act_successful`), link it to the subtask via
  `nrel_executes`, and link the result to it via `nrel_generated_by`;
- create an `concept_audit_record` (also classified
  `concept_audit_record_execution`), link the execution act to it via
  `nrel_recorded_in`;
- create two separate `concept_provenance_record` nodes — one for the
  result, one for the audit record — each linked via `nrel_has_provenance`
  from its subject and via `nrel_recorded_by_sc_agent` to
  `agent_cooperative_executor`;
- move `nrel_subtask_status(σ)` to `completed` (taking into account that the relation is functional — the old arc is erased rather than duplicated, `SetFunctionalRelationTarget`);
- correctly finish with an error when any of the four mandatory arguments is missing, or when either gate above fails.

**Limitations (per code):**
- **does not fill** `wasGeneratedBy`/`wasAttributedTo` (PROV-O part of the same template, `axioms_provenance.scs`) — per the rule text this is the responsibility zone of `agent_provenance_registrar`, not `agent_cooperative_executor`; the `concept_provenance_record` created here is minimal (typed + `recorded_by_sc_agent`), not a full PROV-O Entity/Activity/Agent triplet.
- **does not check the subtask's plan version, use a real execution
  timestamp, or verify transfer integrity of the result** — these
  further items from AGENTS_AUDIT.md §6.2 are outside the minimal set
  the audit itself calls mandatory ("как минимум") and were not
  implemented.
- **does not check argument types beyond the two gates** (only
  `IsElement` for `result`/`context`) — arbitrary nodes can still be
  linked as "result"/"context" without checking membership in the
  corresponding ontology classes.
- **does not deduplicate** provenance links — repeated invocation with the same arguments (e.g. a retried call after the first one already completed the subtask) will fail the `in_progress` gate the second time rather than silently duplicating records, but two *different* subtasks legitimately re-entering `in_progress` would each get their own execution act/audit record/provenance chain, as intended.

## Input

**Trigger:** arc `action_initiated -> <instance of action_register_partial_result>`.

| # | Role | Type | Mandatory | Validation |
|---|---|---|---|---|
| 1 | `system` | `concept_intelligent_system` s that sent the result | mandatory | `IsElement`; must equal `nrel_assigned_to(subtask)`, otherwise error |
| 2 | `result` | result r (not yet classified) | mandatory | only `IsElement` |
| 3 | `subtask` | subtask σ for which the result was received | mandatory | `IsElement`; `nrel_subtask_status(subtask)` must be `concept_subtask_status_in_progress`, otherwise error |
| 4 | `context` | interaction context C | mandatory | only `IsElement` |

## Output

Classification `result <- concept_partial_result` and `result <-
concept_processing_result`; three new provenance links
`produces(system, result)`, `caused_by_subtask(result, subtask)`,
`generated_in_context(result, context)`; a new `concept_execution_act`
(a, also `concept_execution_act_successful`) with `nrel_executes(a,
subtask)` and `nrel_generated_by(result, a)`; a new
`concept_audit_record` (υ, also `concept_audit_record_execution`) with
`nrel_recorded_in(a, υ)`; a `concept_provenance_record` each for
`result` and `υ` (linked via `nrel_has_provenance` and
`nrel_recorded_by_sc_agent -> agent_cooperative_executor`);
`nrel_subtask_status(subtask)` moved to
`concept_subtask_status_completed`. Success — `FinishSuccessfully()` +
`FormResult(result, subtask, a, υ)`.

## Relations

**Dependencies:** `common/relation_io.hpp` (`TargetOf` — for the two
admission gates, `LinkByRelation`, `SetFunctionalRelationTarget` —
moved here while implementing this agent together with
[[AgentPropagateToDependent]], a generalization of the local
`SetFunctionalStatus` from [[AgentDispatchReadySubtask]]),
`keynodes/task_keynodes.hpp` (`TaskKeynodes`, extended with
`concept_processing_result`, `concept_execution_act(_successful)`,
`nrel_executes`, `concept_audit_record(_execution)`, `nrel_recorded_in`,
`concept_provenance_record`, `nrel_has_provenance`, `nrel_generated_by`,
`nrel_recorded_by_sc_agent`).

**Reads:** `nrel_assigned_to(subtask)` and `nrel_subtask_status(subtask)`
for the two admission gates.

**Writes:** classification of `result` (partial + processing result);
the provenance triplet; the execution act, audit record, and both
provenance records; functional `nrel_subtask_status` of the subtask.

**Workflow.** Second step of the subtask lifecycle in `agent_cooperative_executor`: [[AgentDispatchReadySubtask]] (`proposed → in_progress`) → **AgentRegisterPartialResult** (`in_progress → completed` + provenance) → [[AgentPropagateToDependent]] (dispatching the result to dependent subtasks). Linked by the shared state `nrel_subtask_status` of a specific subtask, not by an orchestrator.

## Behavior

```
IF  any of {system, result, subtask, context} is not an sc-element
THEN  FinishWithError()

ELSE IF  TargetOf(subtask, nrel_assigned_to) != system
THEN  FinishWithError()   // system is not the assigned executor

ELSE IF  TargetOf(subtask, nrel_subtask_status) != concept_subtask_status_in_progress
THEN  FinishWithError()   // subtask is not in_progress

ELSE
  result <- concept_partial_result
  result <- concept_processing_result
  LinkByRelation(system, result, nrel_produces)
  LinkByRelation(result, subtask, nrel_caused_by_subtask)
  LinkByRelation(result, context, nrel_generated_in_context)

  a := create a node of class concept_execution_act,
       also classified concept_execution_act_successful
  LinkByRelation(a, subtask, nrel_executes)

  audit := create a node of class concept_audit_record,
         also classified concept_audit_record_execution
  LinkByRelation(a, audit, nrel_recorded_in)

  LinkByRelation(result, a, nrel_generated_by)

  result_prov := create a node of class concept_provenance_record
  LinkByRelation(result, result_prov, nrel_has_provenance)
  LinkByRelation(result_prov, agent_cooperative_executor, nrel_recorded_by_sc_agent)

  audit_prov := create a node of class concept_provenance_record
  LinkByRelation(audit, audit_prov, nrel_has_provenance)
  LinkByRelation(audit_prov, agent_cooperative_executor, nrel_recorded_by_sc_agent)

  SetFunctionalRelationTarget(subtask, nrel_subtask_status,
                               concept_subtask_status_completed)
    // functional relation: the old arc is erased before creating the new one

  action.FormResult(result, subtask, a, audit)
  → action.FinishSuccessfully()
```

**Error handling/timeouts:** same as other agents — `DoProgram` is synchronous; waiting is on the caller side via `InitiateAndWait`.

## Security

- Privileges — same as other agents in `TrustModule`: no ACL, full access to sc-memory within the `sc-server` process.
- **Specific risk:** the two admission gates check that `system` matches the subtask's *recorded* `nrel_assigned_to` and that the subtask is `in_progress` — but they trust the identity of `system` itself: nothing authenticates that the action was actually initiated by that system, the same project-wide absence of authentication for the action initiator. Anyone who can initiate `action_register_partial_result` and knows (or guesses) the assigned system's sc-address can pass the gates.
- `result`/`context` are still only checked with `IsElement`, not verified against their expected ontology classes.
- Data (system identifiers, result content) — not PII directly, but `result` may carry real user data, and `generated_in_context` links it to an interaction context; the new audit/provenance records also reference `agent_cooperative_executor`, not the calling system, as the recorder.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; constant number of operations (does not depend on KB size). |
| Accuracy | Not applicable — deterministic fact recording, not prediction; correctness verified by tests. |
| Cost | Negligible. |
| Alerts | Not implemented. |

## Tests

File: `tests/test_agent_register_partial_result.cpp` (4 tests, all passing). Also covered by the second `TEST_F` in `tests/test_integration_request_pipeline.cpp` (`DispatchesRegistersAndPropagatesAlongSubtaskChain`), where it acts as the middle link in the Dispatch → Register → Propagate chain.

**1. `RegistersPartialResultAndCompletesSubtask`**
Input: `system` linked as `nrel_assigned_to(subtask)`, subtask in status
`in_progress`, all 4 arguments are existing nodes.
Output: `result` classified as both `concept_partial_result` and
`concept_processing_result`; all three provenance links created with the
expected endpoints; an execution act found via `nrel_generated_by(result)`,
classified `concept_execution_act` and linked `nrel_executes` to the
subtask; an audit record found via `nrel_recorded_in(execution_act)`,
classified `concept_audit_record`; distinct provenance records found via
`nrel_has_provenance` on both `result` and the audit record; subtask
status — `completed` (and unique, functional relation).

**2. `FinishesWithErrorWhenSystemIsNotAssignedExecutor`**
Input: subtask assigned to a different system than the one sending the
result, subtask `in_progress`.
Output: `FinishedWithError`; subtask status remains `in_progress`.

**3. `FinishesWithErrorWhenSubtaskNotInProgress`**
Input: `system` correctly assigned via `nrel_assigned_to`, but subtask
status is `proposed`, not `in_progress`.
Output: `FinishedWithError` — a result cannot be completed before
execution has started.

**4. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
