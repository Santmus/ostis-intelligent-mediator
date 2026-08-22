---
type: agent
tags: [agent, sc-agent, cooperation, state-machine]
action_class: action_dispatch_ready_subtask
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentDispatchReadySubtask

← [[Agent Index]] · from [[Implementation Backlog]] (top-5, #4)

> [!note] One argument, not five
> Unlike other agents in the project, there is no need to pass
> the executor, dependency statuses, etc. as separate arguments — all
> of this is already linked to the subtask itself through existing
> ontology relations (`nrel_assigned_to`, `nrel_subtask_status`,
> `nrel_subtask_depends_on`) and is read from the single passed node.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_dispatch_ready_subtask` |
| Type of the class sc-node | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentDispatchReadySubtask : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_dispatch_ready_subtask.{hpp,cpp}` |
| Role | Implements `rule_dispatch_ready_subtask` (`agent_cooperative_executor`, `rules_cooperation.scs`) |
| KB rule | `rules/rules_cooperation.scs` → `rule_dispatch_ready_subtask` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] — same point about one module per `.dylib`) |

## Purpose

**Problem.** A subtask may be assigned to an executor but not yet ready
for launch until the subtasks it depends on have completed
(`nrel_subtask_depends_on`) — sending it earlier would mean
starting processing without the required input data.

**Goal.** Determine the moment when the subtask is actually ready
(assigned + status "proposed" + all dependencies completed), transition
it to the "in progress" state, and notify the executor with a communicative act.

**Value.** Formalizes the subtask state transition as a verifiable
condition rather than a "probably time" heuristic — the same principle
of deterministic state-machine transition described in
`scenario_request_processing.scs`.

**Stakeholders:** `agent_cooperative_executor` (the conceptual "owner"
of the rule, used as the formal sender of the act), intelligent
executor systems waiting for subtasks.

## Competence

**Capabilities:**
- check the triad of readiness conditions: executor is assigned, status
  is `proposed`, all dependencies are `completed` (an empty dependency
  list is considered vacuously satisfied);
- when satisfied — change status to `in_progress` (taking into account that
  `nrel_subtask_status` is functional: the old arc is removed rather than
  duplicated) and generate a `concept_knowledge_exchange_act` with
  sender/receiver/act_type/payload roles;
- write a boolean result indicator (1.0/0.0) to the receiver node,
  creating an sc-link on first access;
- terminate with an error correctly when mandatory arguments are missing.

**Limitations (by code):**
- **does not fill** `rrel_context`/`rrel_timestamp` in the created
  `concept_knowledge_exchange_act`, although the act definition in
  `concepts/knowledge_exchange.scs` assumes them — the rule does not
  specify a source of context/time, and the agent has no way to obtain
  the current time or actual interaction context.
- **act sender is a fixed `agent_cooperative_executor`**, not a parameter:
  the only implemented variant of the rule "owner" in the
  `concepts/sc_agents.scs` catalog; chosen as an existing formal
  element rather than invented, but this makes the sender a constant
  rather than a real dynamic field.
- **"not ready to launch" is not an error, but a 0 indicator**: the agent is
  designed for speculative calls (for example, by an orchestrator on every
  `proposed` subtask periodically) — an intentional difference from
  [[AgentResolveByTrustWeighted]], where an empty input is an error.
- **does not check for cycles** in the dependency graph — if
  `nrel_subtask_depends_on` forms a cycle, the agent will simply never
  consider such a subtask ready; cycle detection is the responsibility of
  `rule_check_acyclicity` (`rules_decomposition.scs`, not implemented).
- status is read directly as `TargetOf(...) == concept_subtask_status_proposed`
  without checking whether the subtask has any status at all —
  missing status and "wrong" status are indistinguishable (both yield
  `ready = false`).

## Input

**Trigger:** arc `action_initiated -> <instance of action_dispatch_ready_subtask>`.

| # | Role | Type | Mandatory | Validation |
|---|---|---|---|---|
| 1 | `subtask` | existing instance of `concept_subtask` | mandatory | only `IsElement` |
| 2 | `result_level` | receiver node for the indicator | mandatory | only `IsElement` |

## Output

Updated/created sc-link `value_link` on `result_level` with
`1.0` (dispatched) or `0.0` (condition not met). If
dispatched — additionally: change `nrel_subtask_status` of the subtask
to `concept_subtask_status_in_progress`, new
`concept_knowledge_exchange_act` (sender=`agent_cooperative_executor`,
receiver=assigned system, act_type=`concept_act_request`,
payload=subtask). Success — `FinishSuccessfully()` +
`FormResult(subtask, valueLink)`.

## Connections

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`TargetOf`/`LinkByRelation` — moved here
specifically while implementing this agent, since they were needed a second
time after [[AgentResolveByTrustWeighted]]), `keynodes/task_keynodes.hpp`
(`TaskKeynodes`).

**Reads:** `nrel_assigned_to`, `nrel_subtask_status` of the subtask and all
its direct dependencies (`nrel_subtask_depends_on`).

**Writes:** `nrel_subtask_status` of the subtask (when ready),
`concept_knowledge_exchange_act` with linked roles, `value_link`
`result_level`.

**Workflow.** Independent of the other project agents in terms of data —
the only one operating in the "subtask" domain (`concept_subtask`), not
trust/provenance/result integration.

## Behavior

```
IF  subtask is not an sc-element  OR  result_level is not an sc-element
THEN  FinishWithError()

ELSE
  s      := TargetOf(subtask, nrel_assigned_to)
  status := TargetOf(subtask, nrel_subtask_status)

  ready := IsElement(s)
           AND status == concept_subtask_status_proposed
           AND ∀ dep IN subtask -[nrel_subtask_depends_on]->:
                 TargetOf(dep, nrel_subtask_status) == concept_subtask_status_completed
           // empty dependency list -> "for all" is vacuously true

  IF  ready
  THEN
    change nrel_subtask_status(subtask) to concept_subtask_status_in_progress
      // functional relation: old arc is removed before creating the new one
    create concept_knowledge_exchange_act:
      rrel_sender   = agent_cooperative_executor
      rrel_receiver = s
      rrel_act_type = concept_act_request
      rrel_payload  = subtask

  write (ready ? 1.0 : 0.0) to value_link result_level (create if absent)
  action.FormResult(subtask, value_link)
  → action.FinishSuccessfully()
```

**Error/timeout handling:** same as other agents — `DoProgram`
is synchronous; waiting is on the calling side via
`InitiateAndWait`.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full
  access to sc-memory within the `sc-server` process.
- **Specific risk:** the agent can change the status of ANY passed
  subtask to `in_progress` and send a request to ANY system
  specified in `nrel_assigned_to`, without checking whether the calling
  party had the right to do so — the same project-wide absence of
  action initiator authentication, but here with a real side effect
  (communicative act, not just a number).
- It is not checked that `subtask` is actually a `concept_subtask` (only
  `IsElement`) — an arbitrary node can be transitioned to
  "in_progress state" and produce a spurious request.
- Data (statuses, system identifiers) is not directly PII, but
  `concept_knowledge_exchange_act` may contain `rrel_payload` with a
  real user subtask.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; linear in the number of direct subtask dependencies (usually single-digit). |
| Accuracy | Not applicable — boolean condition check, not a prediction; correctness verified by tests for both outcomes (ready/not ready). |
| Cost | Negligible. |
| Alerts | Not implemented. An alert would be appropriate for a subtask that remains `proposed` for years due to an uncompleted dependency (potential deadlock in the graph). |

## Tests

File: `tests/test_agent_dispatch_ready_subtask.cpp` (3 tests, all passing; scenario 1 additionally verified on a live sc-server).

**1. `DispatchesSubtaskWithNoDependencies`**
Input: subtask assigned to a system, status `proposed`, no dependencies.
Output: indicator `1.0`, status changed to `in_progress` (and remains
the only one), `concept_knowledge_exchange_act` created with
`rrel_receiver` = assigned system.

**2. `DoesNotDispatchWhenDependencyIncomplete`**
Input: same subtask, but with one dependency in status `in_progress`
(not `completed`).
Output: indicator `0.0`, subtask status remained `proposed` — unchanged.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action with no arguments.
Output: `FinishedWithError`.
