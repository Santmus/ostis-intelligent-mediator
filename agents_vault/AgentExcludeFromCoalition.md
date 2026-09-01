---
type: agent
tags: [agent, sc-agent, coalition, state-transition]
action_class: action_exclude_from_coalition
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentExcludeFromCoalition

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_on_system_failure`, READY) ·
first agent in the `agent_coalition_reconfigurator` family

> [!note] The mechanical half of a two-part rule
> `rule_on_system_failure` asks for two things: exclude the failed
> system from its coalition, then re-run `rule_coalition_optimization`
> for its orphaned subtasks. Only the first is a deterministic
> state-transition; the second is the same unimplemented combinatorial
> assignment problem that blocks `rules_allocation.scs` throughout the
> project (see [[Implementation Backlog]]). This agent does only the
> first.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_exclude_from_coalition` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentExcludeFromCoalition : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_exclude_from_coalition.{hpp,cpp}` |
| Role | Implements the mechanical part of `rule_on_system_failure` (`agent_coalition_reconfigurator` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_coalition_reconfiguration.scs` → `rule_on_system_failure` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** When a subtask a system was executing fails, that system
should no longer be considered part of the coalition that was relying on
it — otherwise stale membership silently misrepresents who is actually
available.

**Goal.** Given a subtask and a specific coalition, check the rule's
three literal conditions (`subtask_status = failed`, `assigned_to`
resolves to a system, that system is a member of the given coalition)
and, if all hold, erase the membership arc.

**Value.** Keeps `nrel_member_of` — and therefore anything downstream
that reads coalition membership — accurate after a failure, without
requiring the (unimplemented) coalition-optimization step to also exist
first.

**Stakeholders:** `rule_coalition_optimization`'s eventual implementation
(would consume the now-excluded system's freed-up subtasks),
`agent_task_allocator` (not implemented — would need accurate membership
when re-assigning).

## Competence

**Capabilities:**
- resolve the subtask's assignee via `nrel_assigned_to` and its status
  via `nrel_subtask_status`;
- check that the assignee is specifically a member of the *given*
  `community` argument, via a single targeted lookup (`nrel_member_of`
  with both endpoints fixed) rather than scanning all of the assignee's
  memberships;
- erase exactly that membership arc when all three conditions hold;
- write `1.0` (excluded) / `0.0` (a condition was not met — not an
  error) to a `value_link` on `result_level`, same convention as
  [[AgentDispatchReadySubtask]];
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not trigger `rule_coalition_optimization`** — see the note
  above; no algorithm exists in the KB text for it.
- **`community` must be supplied by the caller, not inferred** — the
  agent does not pick "the" coalition of the failed system on its own.
  If the system belongs to several communities, only the one explicitly
  passed is checked/affected — deliberately, to avoid guessing which
  membership the caller actually meant to remove.
- **does not verify `subtask`/`community` are the expected classes** —
  only `IsElement`, same convention as the rest of the module.
- **no compensating action for the orphaned subtask itself** — the
  subtask that failed keeps its `failed` status; nothing here reassigns
  or reschedules it (that's `agent_task_allocator`'s job, not
  implemented).

## Input

**Trigger:** arc `action_initiated -> <instance of action_exclude_from_coalition>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `subtask` | `ScAddr` of the subtask σ whose status is checked | required | only `IsElement` |
| 2 | `community` | `ScAddr` of the specific coalition K to check/exclude from | required | only `IsElement` |
| 3 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level` with
`1.0` (excluded) or `0.0` (no-op — a condition was not met). If excluded,
the `system -[nrel_member_of]-> community` arc no longer exists. Success —
`FinishSuccessfully()` + `FormResult(subtask, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`TargetOf`), `keynodes/coalition_keynodes.hpp`
(new — `CoalitionKeynodes`, shared with [[AgentReassignOnResourceExhaustion]]).

**Reads:** `nrel_assigned_to`/`nrel_subtask_status` of `subtask`, the
targeted `nrel_member_of` arc between the resolved system and `community`.

**Writes:** erases the `nrel_member_of` arc on success; the `value_link`
of `result_level` always.

**Workflow.** Data-independent from every other agent in the module —
the only one operating on `nrel_member_of`. Not wired to any coalition
reoptimization step, which does not exist.

## Behavior

```
IF  subtask is not an sc-element  OR  community is not an sc-element
    OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  system := TargetOf(subtask, nrel_assigned_to)
  status := TargetOf(subtask, nrel_subtask_status)

  excluded := false
  IF  system is an sc-element  AND  status == concept_subtask_status_failed
  THEN
    IF  ∃ arc system -[nrel_member_of]-> community
    THEN
      erase that arc
      excluded := true

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (excluded ? 1.0 : 0.0) to value_link
  action.FormResult(subtask, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument; any
unmet precondition (not failed, no assignee, not a member of the given
community) is a normal, non-error outcome (`excluded = false`).

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the agent erases a real membership arc based
  entirely on caller-supplied `subtask`/`community` — a caller with write
  access could exclude an arbitrary system from an arbitrary coalition by
  fabricating a `failed`-status subtask and an `assigned_to` arc pointing
  at that system, with no further check of legitimacy.
- No PII involved — the data is structural (subtask/system/coalition
  identity).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — two `TargetOf` lookups plus one targeted `nrel_member_of` check, independent of KB size. |
| Accuracy | Not applicable — deterministic condition check; verified by tests covering each of the three conditions failing independently. |
| Cost | Negligible. |
| Alerts | None — the indicator is a passive, queryable value; nothing consumes it proactively (the reoptimization step it should trigger does not exist). |

## Tests

File: `tests/test_agent_exclude_from_coalition.cpp` (4 tests, all pass).

**1. `ExcludesSystemWhenAllConditionsHold`**
Input: subtask `failed`, `assigned_to` a system that is a member of the
given community.
Output: indicator `1.0`, the membership arc no longer exists.

**2. `DoesNotExcludeWhenSubtaskNotFailed`**
Input: same setup, but subtask status is `reassigned`, not `failed`.
Output: indicator `0.0`, membership untouched.

**3. `DoesNotExcludeWhenNotMemberOfGivenCommunity`**
Input: subtask `failed` and assigned, but the system's actual membership
is in a *different* community than the one passed to the action.
Output: indicator `0.0`, the system's real membership is left intact —
confirms the agent checks the specific `community` argument, not "any"
membership.

**4. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
