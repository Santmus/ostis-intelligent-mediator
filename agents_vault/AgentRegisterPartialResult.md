---
type: agent
tags: [agent, sc-agent, cooperation, provenance]
action_class: action_register_partial_result
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentRegisterPartialResult

← [[Agent Index]] · from [[Implementation Backlog]] (`rules_cooperation.scs`, after [[AgentDispatchReadySubtask]])

> [!note] No readiness indicator
> Unlike [[AgentDispatchReadySubtask]] and [[AgentPropagateToDependent]],
> there is no readiness condition — the rule unconditionally records the fact
> of receiving the result. Therefore, the agent is non-speculative and does not write
> a numeric indicator into the receiving node: the only path to failure —
> a missing argument (`FinishWithError`).

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

**Goal.** Atomically record the provenance triplet `produces`/`caused_by_subtask`/`generated_in_context`, classify the result as `concept_partial_result`, and transition the subtask to `completed` — in a single, deterministic step.

**Value.** Implements the same logic described in `axioms/axioms_provenance.scs` (the automatic instantiation template when executing a subtask) and reproduced as a fixture in `scenario_request_processing.scs` (step 5, `result_routes`).

**Stakeholders:** `agent_cooperative_executor` (conceptual "owner" of the rule), [[AgentPropagateToDependent]] (next step in the same subtask lifecycle — reads the `completed` status set here), `agent_provenance_registrar` (augments the same result with PROV-O links `wasGeneratedBy`/`wasAttributedTo` — outside this agent's scope).

## Competence

**Capabilities:**
- classify the passed result node as `concept_partial_result`;
- create three non-role binary relations: `produces(s, r)`, `caused_by_subtask(r, σ)`, `generated_in_context(r, C)`;
- move `nrel_subtask_status(σ)` to `completed` (taking into account that the relation is functional — the old arc is erased rather than duplicated, `SetFunctionalRelationTarget`);
- correctly finish with an error when any of the four mandatory arguments is missing.

**Limitations (per code):**
- **does not fill** `wasGeneratedBy`/`wasAttributedTo` (PROV-O part of the same template, `axioms_provenance.scs`) — per the rule text this is the responsibility zone of `agent_provenance_registrar`, not `agent_cooperative_executor`.
- **unconditional**: does not check that the subtask was actually in status `in_progress` before completion — the rule does not formulate such a precondition, and the agent merely records the fact "result received".
- **does not check argument types** (only `IsElement`) — arbitrary nodes can be linked as "system"/"result"/"subtask"/"context" without checking membership in the corresponding ontology classes.
- **does not deduplicate** provenance links — repeated invocation with the same arguments will create second instances of `produces`/`caused_by_subtask`/`generated_in_context` (non-functional relations, unlike status), a potential source of duplicates during non-idempotent external calls.

## Input

**Trigger:** arc `action_initiated -> <instance of action_register_partial_result>`.

| # | Role | Type | Mandatory | Validation |
|---|---|---|---|---|
| 1 | `system` | `concept_intelligent_system` s that sent the result | mandatory | only `IsElement` |
| 2 | `result` | result r (not yet classified) | mandatory | only `IsElement` |
| 3 | `subtask` | subtask σ for which the result was received | mandatory | only `IsElement` |
| 4 | `context` | interaction context C | mandatory | only `IsElement` |

## Output

Classification `result <- concept_partial_result`; three new links `produces(system, result)`, `caused_by_subtask(result, subtask)`, `generated_in_context(result, context)`; `nrel_subtask_status(subtask)` moved to `concept_subtask_status_completed`. Success — `FinishSuccessfully()` + `FormResult(result, subtask)`.

## Relations

**Dependencies:** `common/relation_io.hpp` (`LinkByRelation`, `SetFunctionalRelationTarget` — moved here while implementing this agent together with [[AgentPropagateToDependent]], a generalization of the local `SetFunctionalStatus` from [[AgentDispatchReadySubtask]]), `keynodes/task_keynodes.hpp` (`TaskKeynodes`).

**Reads:** nothing (arguments are accepted as ready sc-elements; there are no preconditions on their current state).

**Writes:** classification of `result`, three `nrel_*` links, functional `nrel_subtask_status` of the subtask.

**Workflow.** Second step of the subtask lifecycle in `agent_cooperative_executor`: [[AgentDispatchReadySubtask]] (`proposed → in_progress`) → **AgentRegisterPartialResult** (`in_progress → completed` + provenance) → [[AgentPropagateToDependent]] (dispatching the result to dependent subtasks). Linked by the shared state `nrel_subtask_status` of a specific subtask, not by an orchestrator.

## Behavior

```
IF  any of {system, result, subtask, context} is not an sc-element
THEN  FinishWithError()

ELSE
  result <- concept_partial_result
  LinkByRelation(system, result, nrel_produces)
  LinkByRelation(result, subtask, nrel_caused_by_subtask)
  LinkByRelation(result, context, nrel_generated_in_context)

  SetFunctionalRelationTarget(subtask, nrel_subtask_status,
                               concept_subtask_status_completed)
    // functional relation: the old arc is erased before creating the new one

  action.FormResult(result, subtask)
  → action.FinishSuccessfully()
```

**Error handling/timeouts:** same as other agents — `DoProgram` is synchronous; waiting is on the caller side via `InitiateAndWait`.

## Security

- Privileges — same as other agents in `TrustModule`: no ACL, full access to sc-memory within the `sc-server` process.
- **Specific risk:** the agent unconditionally transitions ANY passed subtask to `completed` and classifies ANY node as a result, without verifying that the caller is indeed the assigned system — the same project-wide absence of authentication for the action initiator.
- Data (system identifiers, result content) — not PII directly, but `result` may carry real user data, and `generated_in_context` links it to an interaction context.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; constant number of operations (does not depend on KB size). |
| Accuracy | Not applicable — deterministic fact recording, not prediction; correctness verified by tests. |
| Cost | Negligible. |
| Alerts | Not implemented. |

## Tests

File: `tests/test_agent_register_partial_result.cpp` (3 tests, all passing). Also covered by the second `TEST_F` in `tests/test_integration_request_pipeline.cpp` (`DispatchesRegistersAndPropagatesAlongSubtaskChain`), where it acts as the middle link in the Dispatch → Register → Propagate chain.

**1. `RegistersPartialResultAndCompletesSubtask`**
Input: subtask in status `in_progress`, all 4 arguments are existing nodes.
Output: `result` classified as `concept_partial_result`, all three provenance links created with the expected endpoints, subtask status — `completed` (and unique).

**2. `SetsCompletedEvenWithoutPriorStatus`**
Input: subtask with no prior `nrel_subtask_status` arc at all.
Output: status is still set to `completed` — confirms the agent's unconditionality.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
