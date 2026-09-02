---
type: agent
tags: [agent, sc-agent, allocation, state-transition]
action_class: action_handle_no_admissible_assignment
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentHandleNoAdmissibleAssignment

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_handle_no_admissible_assignment`, READY) ·
first agent implemented for `agent_task_allocator` / `rules_allocation.scs`

> [!note] The "no admissible executor" fact is an input, not a computation
> The rule triggers on "∀s ∈ S. ¬(match(σ,s) ≥ θ ∧ valid(C,L_s))" — but
> `match()` is only partially defined (`rule_compute_match_function`
> names six terms; only `trust(s,τ,h)` is actually computable today, see
> [[Implementation Backlog]]). This agent does not re-derive the
> premise — `subtask` is passed in already established as having no
> admissible executor, the same convention as
> [[AgentExcludeFromCoalition]]/[[AgentReassignOnResourceExhaustion]]
> trusting `failed`/`resource_exhausted`.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_handle_no_admissible_assignment` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentHandleNoAdmissibleAssignment : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_handle_no_admissible_assignment.{hpp,cpp}` |
| Role | Implements the mechanical part of `rule_handle_no_admissible_assignment` (`agent_task_allocator` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_allocation.scs` → `rule_handle_no_admissible_assignment` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** When no system is an admissible executor for a subtask,
the rule offers two paths — recursive decomposition, or an integrity
violation — but nothing decided which applies, or created the
violation when it did.

**Goal.** Given a subtask already known to have no admissible executor,
decide which of the rule's two paths applies (by checking whether the
subtask is composite or atomic, per `affirmation_subtask_executability`:
every subtask is either executable or decomposable) and, for the atomic
case, create the integrity violation.

**Value.** Turns the rule's branching logic into an executable check,
and gives `agent_task_allocator`'s failure path a concrete, testable
outcome even though the two things it might trigger — actual recursive
decomposition, an orchestrator to resume control — are not implemented.

**Stakeholders:** `agent_task_decomposer`'s eventual
`rule_apply_decomposition_pattern` (would consume the composite branch —
not implemented, see Limitations), whatever lifecycle orchestrator
would eventually "return control to agent_task_decomposer" (not
implemented, [[Agent Index]] §7.1).

## Competence

**Capabilities:**
- check whether `subtask` is classified `concept_subtask_composite`
  (`concepts/tasks.scs`);
- if composite, report success without creating a violation (the
  decomposition path is applicable — not performed, see Limitations);
- if not (atomic), create a `concept_integrity_violation` node, attach
  it under `result_level`, and link it to `subtask` via `nrel_concerns`
  (same violation-creation pattern as [[AgentValidateCardinalityConstraint]]/
  [[AgentCheckAcyclicity]]);
- write `1.0`/`0.0` into a `value_link` on `result_level` (`1.0` =
  decomposable, no violation; `0.0` = atomic, violation raised — the
  usual "1.0 = no problem" polarity);
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not verify the "no admissible executor" premise** — see the
  note above; `match(σ,s)` is not recomputed for any system.
- **does not trigger `rule_recursive_decomposition`** — the agent only
  recognizes that the decomposition path is *applicable* for a
  composite subtask; the actual decomposition
  (`rule_apply_decomposition_pattern`) is not implemented (needs
  `sim_T` and a template library undefined in the text).
- **does not "return control to agent_task_decomposer"** — no lifecycle
  orchestrator exists in the project ([[Agent Index]] §7.1).
- **generic violation class only** — unlike
  [[AgentValidateRightToExplanation]]'s dual classification, this
  rule's text names no specific violation subtype, so the node is
  classified only `concept_integrity_violation`, nothing more specific.
- only `IsElement` on arguments; `subtask`'s base class
  (`concept_subtask`) is not verified, only its `concept_subtask_composite`
  classification is read — same convention as the rest of the module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_handle_no_admissible_assignment>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `subtask` | `ScAddr` of σ, already established to have no admissible executor | required | only `IsElement` |
| 2 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level`:
`1.0` if `subtask` is composite (decomposable), `0.0` if atomic. On
`0.0`, a `concept_integrity_violation` node is also attached under
`result_level` and linked to `subtask` via `nrel_concerns`. Success —
`FinishSuccessfully()` + `FormResult(subtask, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`LinkByRelation`), `keynodes/allocation_keynodes.hpp`
(`AllocationKeynodes` — new file, first agent of this domain, same
minimal-start pattern as `coalition_keynodes.hpp`/`adaptive_interface_keynodes.hpp`;
`concept_integrity_violation` duplicated here the same way
`SecurityKeynodes` duplicates it), `keynodes/integrity_keynodes.hpp`
(`IntegrityKeynodes::nrel_concerns`, reused).

**Reads:** classification of `subtask` (`concept_subtask_composite`).

**Writes:** on the atomic branch, one violation node + its classifying
arc + its `nrel_concerns` arc; the `value_link` of `result_level`.

**Workflow.** Data-independent from every other agent in the module —
the first and only agent operating in the `agent_task_allocator`
domain.

## Behavior

```
IF  subtask is not an sc-element  OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  decomposable := subtask classified concept_subtask_composite

  IF NOT decomposable:
    create violation (ConstNode) classified concept_integrity_violation
    attach violation under result_level
    LinkByRelation(violation, subtask, nrel_concerns)

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (decomposable ? 1.0 : 0.0) to value_link
  action.FormResult(subtask, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument;
both the composite and atomic branches are normal, non-error outcomes.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the agent trusts the caller's claim that `subtask`
  genuinely has no admissible executor — nothing verifies this before
  flagging an integrity violation (atomic case) or silently accepting
  the decomposition path (composite case), which is a real disruption
  if the claim is wrong.
- No PII involved — the data is structural (subtask identity and
  classification).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — one classification check, at most one violation construction. |
| Accuracy | Not applicable — deterministic binary branch; verified by tests covering both the composite and atomic cases. |
| Cost | Negligible. |
| Alerts | None — the violation is a passive, queryable node; nothing consumes it proactively (no lifecycle orchestrator, see Limitations). |

## Tests

File: `tests/test_agent_handle_no_admissible_assignment.cpp` (3 tests, all pass).

**1. `CreatesViolationForAtomicSubtask`**
Input: subtask not classified `concept_subtask_composite`.
Output: indicator `0.0`; violation created, `nrel_concerns` points to
`subtask`.

**2. `DoesNotCreateViolationForCompositeSubtask`**
Input: subtask classified `concept_subtask_composite`.
Output: indicator `1.0`; no violation.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
