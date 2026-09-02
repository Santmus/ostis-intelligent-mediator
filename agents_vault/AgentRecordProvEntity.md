---
type: agent
tags: [agent, sc-agent, provenance, prov-o]
action_class: action_record_prov_entity
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentRecordProvEntity

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_auto_record_prov_entity`, READY) ·
first agent to populate the PROV-O triad (`concept_prov_entity`/`_activity`/`_agent`),
sibling of [[AgentRecordProvActivity]]

> [!note] Explicit action, not an on-appearance trigger
> The rule reads "When: new concept_processing_result(r) appears in
> sc-memory". No agent in this project subscribes to a generic
> node-creation event — every agent is `ScActionInitiatedAgent`,
> reacting to an explicit `action_initiated` arc. This agent is invoked
> explicitly by a caller, the same convention as everywhere else in the
> module.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_record_prov_entity` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentRecordProvEntity : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_record_prov_entity.{hpp,cpp}` |
| Role | Implements `rule_auto_record_prov_entity` (`agent_provenance_registrar` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_provenance_completion.scs` → `rule_auto_record_prov_entity` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `concepts/provenance.scs` fully formalizes the PROV-O triad
(`concept_prov_entity`/`_activity`/`_agent` and their relations, aligned
with W3C PROV-O), but nothing in the project populated it — every class
and relation this rule needs already existed, unused.

**Goal.** Given a processing result and the activity/agent that produced
it, create a PROV entity for the result and link it via `wasGeneratedBy`/
`wasAttributedTo`/`has_provenance`.

**Value.** Turns the PROV-O subsystem from declared-but-empty into
populated data — and, per its own Limitations, is also what
[[Implementation Backlog]] identified as the missing prerequisite for
`rule_link_explanation_to_provenance` (which needs a typed
`concept_prov_activity` for the explanation act, previously blocked on
exactly this gap).

**Stakeholders:** [[AgentRecordProvActivity]] (typically supplies the
`activity` argument), any future consumer of a fully PROV-O-typed
provenance graph (e.g., `rule_link_explanation_to_provenance`, still
otherwise blocked — see its own note in [[Implementation Backlog]]).

## Competence

**Capabilities:**
- create a fresh `concept_prov_entity` node for `result`;
- link it to `activity` via `nrel_prov_was_generated_by` and to `agent`
  via `nrel_prov_was_attributed_to`;
- set it as `result`'s `nrel_has_provenance` target via
  `SetFunctionalRelationTarget` (the relation is functional — any
  existing target, from this agent or any other source, is replaced,
  not supplemented);
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not create `activity`/`agent`** — both must already exist
  (typically `activity` via [[AgentRecordProvActivity]]); no attempt is
  made to verify they are actually classified `concept_prov_activity`/
  `concept_prov_agent`, same trust-the-caller convention as the rest of
  the module.
- **does not fire automatically on result creation** — see the note
  above.
- **overlaps with [[AgentRegisterPartialResult]]'s existing provenance
  mechanism, observation only:** that agent (from `rules_cooperation.scs`)
  already creates a generic `concept_provenance_record` node (not
  PROV-O-typed) and sets it as the result's `nrel_has_provenance`
  target. Since that relation is functional, calling this agent AFTER
  [[AgentRegisterPartialResult]] replaces the generic record with the
  PROV-O-specific entity (`SetFunctionalRelationTarget` erases the old
  arc). Nothing in the model text says which mechanism is canonical —
  this is a documented tension, not resolved here.
- only `IsElement` on arguments, same convention as the rest of the
  module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_record_prov_entity>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `result` | `ScAddr` of r, the `concept_processing_result` instance | required | only `IsElement` |
| 2 | `activity` | `ScAddr` of the producing `concept_prov_activity` instance | required | only `IsElement` |
| 3 | `agent` | `ScAddr` of the producing `concept_prov_agent` instance | required | only `IsElement` |

## Output

A new `concept_prov_entity` node, linked to `activity`/`agent` and set
as `result`'s `nrel_has_provenance` target. Success —
`FinishSuccessfully()` + `FormResult(result, entity)`.

## Links

**Dependencies:** `common/relation_io.hpp` (`LinkByRelation`,
`SetFunctionalRelationTarget`), `keynodes/provenance_keynodes.hpp`
(`ProvenanceKeynodes` — extended with the full PROV-O vocabulary:
`concept_prov_entity`/`_activity`, `nrel_prov_was_generated_by`/
`_was_attributed_to`/`_used`/`_was_associated_with`, `rrel_actor`/
`_input`/`_timestamp`, alongside the pre-existing
`nrel_has_provenance`/`nrel_derived_from`).

**Reads:** nothing beyond argument identity (no structural traversal).

**Writes:** one `concept_prov_entity` node + two classifying/relation
arcs (`wasGeneratedBy`, `wasAttributedTo`) + the functional
`nrel_has_provenance` arc on `result`.

**Workflow.** Data-independent from every other agent except the
functional-relation overlap with [[AgentRegisterPartialResult]] noted
above. Typically follows [[AgentRecordProvActivity]] in a caller's
pipeline (activity created first, then referenced here), but the two
are not directly coupled — either can be called independently.

## Behavior

```
IF  result is not an sc-element
 OR activity is not an sc-element
 OR agent is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  entity := GenerateNode(ConstNode) classified concept_prov_entity
  LinkByRelation(entity, activity, nrel_prov_was_generated_by)
  LinkByRelation(entity, agent, nrel_prov_was_attributed_to)
  SetFunctionalRelationTarget(result, nrel_has_provenance, entity)

  action.FormResult(result, entity)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument;
there is no other failure mode — the rule describes pure construction.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — pure construction
  from caller-supplied identities; a malicious caller could attribute a
  result to an arbitrary `agent`/`activity`, but nothing in the module
  verifies producer identity anywhere else either.
- No PII involved — the data is structural (result/activity/agent
  identity).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — three connector generations, one functional-relation replace. |
| Accuracy | Not applicable — deterministic construction; verified by tests including a functional-replace case. |
| Cost | Negligible. |
| Alerts | None. |

## Tests

File: `tests/test_agent_record_prov_entity.cpp` (3 tests, all pass).

**1. `RecordsEntityWithGeneratedByAndAttributedTo`**
Input: result, activity, agent all fresh nodes.
Output: a new `concept_prov_entity` reachable via `result`'s
`nrel_has_provenance`, with `wasGeneratedBy`/`wasAttributedTo` pointing
at `activity`/`agent`.

**2. `ReplacesExistingProvenanceTargetFunctionally`**
Input: two successive calls on the same `result` with different
activity/agent pairs.
Output: exactly one `nrel_has_provenance` arc remains afterward, now
pointing at the second call's entity — confirms the old arc was erased,
not supplemented.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
