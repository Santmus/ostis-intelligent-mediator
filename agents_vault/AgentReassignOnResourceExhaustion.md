---
type: agent
tags: [agent, sc-agent, coalition, state-transition]
action_class: action_reassign_on_resource_exhaustion
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentReassignOnResourceExhaustion

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_on_resource_exhaustion`, READY) ·
sibling of [[AgentExcludeFromCoalition]] in the `agent_coalition_reconfigurator` family

> [!note] The system's resource violation is an input, not a computation
> The rule triggers "on detecting a violation of L_s" (`resource_exhausted`),
> but nowhere defines that detection numerically. This agent does not try
> to compute it — `system` is passed in already identified as exhausted;
> the agent's own job starts after that fact is established.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_reassign_on_resource_exhaustion` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentReassignOnResourceExhaustion : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_reassign_on_resource_exhaustion.{hpp,cpp}` |
| Role | Implements the mechanical part of `rule_on_resource_exhaustion` (`agent_coalition_reconfigurator` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_coalition_reconfiguration.scs` → `rule_on_resource_exhaustion` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** When a system runs out of a declared resource, every
subtask still assigned to it needs to be flagged for reassignment —
otherwise the ecosystem keeps treating those subtasks as if they were
progressing normally.

**Goal.** Given a system, find every subtask currently assigned to it and
set each one's status to `concept_subtask_status_reassigned`.

**Value.** Makes resource exhaustion visible system-wide through the
existing `nrel_subtask_status` mechanism, without needing a caller to
already know (and pass in) the affected subtask set — the agent derives
it itself via a reverse `nrel_assigned_to` lookup.

**Stakeholders:** `rule_coalition_optimization`'s eventual
implementation (would consume the now-`reassigned` subtasks),
`agent_task_allocator` (not implemented — would need to actually pick new
assignees for them).

## Competence

**Capabilities:**
- find every subtask with an outgoing `nrel_assigned_to` arc to the given
  `system`, via a single reverse-direction iterator (no caller-supplied
  group needed — contrast with [[AgentComputePiMetric]]'s `elements_group`
  convention, not used here because the reverse index is directly
  queryable);
- set each such subtask's status to `concept_subtask_status_reassigned`
  via `SetFunctionalRelationTarget` (erases the old status arc first —
  `nrel_subtask_status` is functional);
- write the count of reassigned subtasks as a float into a `value_link`
  on `result_level`;
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not detect or validate `resource_exhausted` itself** — see the
  note above; `system` is accepted unconditionally as already
  identified as exhausted.
- **does not trigger `rule_coalition_optimization`** — same unimplemented
  combinatorial problem noted in [[AgentExcludeFromCoalition]].
- **reassigned subtasks get no new assignee** — the status change alone
  marks them as needing one; nothing here performs (or can perform, since
  `agent_task_allocator` doesn't exist) the actual reassignment.
- **does not verify `system` is the expected class** — only `IsElement`,
  same convention as the rest of the module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_reassign_on_resource_exhaustion>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `system` | `ScAddr` of the system s whose resource constraint was violated | required | only `IsElement` |
| 2 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level` with
the count of subtasks whose status was set to `concept_subtask_status_reassigned`
(`0.0` if none were assigned — not an error). Success —
`FinishSuccessfully()` + `FormResult(system, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`SetFunctionalRelationTarget`),
`keynodes/coalition_keynodes.hpp` (`CoalitionKeynodes`, shared with
[[AgentExcludeFromCoalition]]).

**Reads:** incoming `nrel_assigned_to` arcs to `system`.

**Writes:** `nrel_subtask_status` of every subtask found (replacing any
existing status); the `value_link` of `result_level`.

**Workflow.** Data-independent from every other agent in the module.
Mutates while iterating the same pattern already used by
[[AgentPropagateToDependent]] (accumulate a float count while performing
a per-item side effect on a *different* relation than the one being
iterated) — safe because `nrel_subtask_status` mutations never touch the
`nrel_assigned_to` arcs the iterator walks.

## Behavior

```
IF  system is not an sc-element  OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  count := 0
  FOR EACH subtask WITH outgoing nrel_assigned_to -> system:
    SetFunctionalRelationTarget(subtask, nrel_subtask_status, concept_subtask_status_reassigned)
    count += 1

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write count to value_link
  action.FormResult(system, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument; a
system with no assigned subtasks is a normal, non-error outcome
(`count = 0`).

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the agent trusts the caller's claim that `system` is
  genuinely resource-exhausted — nothing verifies this before mass-marking
  every one of its subtasks `reassigned`, which is a real disruption if
  the claim is wrong or malicious.
- No PII involved — the data is structural (system/subtask identity and
  status).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(N) in the number of subtasks currently assigned to `system` — one `CreateIterator5` scan plus one status update per match. |
| Accuracy | Not applicable — deterministic bulk update; verified by tests including a multi-subtask case and a case with an unrelated system's subtask left untouched. |
| Cost | Negligible for typical assignment counts. |
| Alerts | None — the count is a passive, queryable value; nothing consumes it proactively (the reoptimization step it should trigger does not exist). |

## Tests

File: `tests/test_agent_reassign_on_resource_exhaustion.cpp` (4 tests, all pass).

**1. `ReassignsAllSubtasksAssignedToSystem`**
Input: three subtasks assigned to `system`, one unrelated subtask
assigned to a different system.
Output: indicator `3.0`; all three get status `reassigned`; the unrelated
subtask's status is untouched (still absent).

**2. `ReplacesExistingStatusFunctionally`**
Input: a subtask already `failed` and assigned to `system`.
Output: exactly one `nrel_subtask_status` arc remains afterward, now
pointing to `reassigned` — confirms the old arc was erased, not just
supplemented.

**3. `ReportsZeroWhenNoSubtasksAssigned`**
Input: `system` with no assigned subtasks at all.
Output: indicator `0.0`, `FinishedSuccessfully`.

**4. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
