---
type: agent
tags: [agent, sc-agent, provenance, prov-o]
action_class: action_record_prov_activity
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentRecordProvActivity

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_auto_record_prov_activity`, READY) ·
sibling of [[AgentRecordProvEntity]] in populating the PROV-O triad

> [!note] Explicit action, not an on-appearance trigger
> The rule reads "When: sc-agent s_a executes process π_i on input
> in_i" — same deliberate departure as [[AgentRecordProvEntity]]: no
> agent in this project reacts to a generic process-execution event,
> only to an explicit `action_initiated` arc.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_record_prov_activity` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentRecordProvActivity : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_record_prov_activity.{hpp,cpp}` |
| Role | Implements `rule_auto_record_prov_activity` (`agent_provenance_registrar` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_provenance_completion.scs` → `rule_auto_record_prov_activity` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** Same as [[AgentRecordProvEntity]]: `concept_prov_activity`
and its relations (`nrel_prov_used`, `nrel_prov_was_associated_with`)
were fully typed in `concepts/provenance.scs` but never populated by any
agent.

**Goal.** Given an actor, an input entity, and a PROV agent, reify a
PROV activity with the roles/relations the rule names.

**Value.** Together with [[AgentRecordProvEntity]], gives the PROV-O
subsystem an actual population path — the pair is what
`rule_link_explanation_to_provenance` was blocked on (see
[[Implementation Backlog]]'s own note on that rule), closed the next
day by [[AgentLinkExplanationToProvenance]].

**Stakeholders:** [[AgentRecordProvEntity]] (typically consumes this
agent's output as its `activity` argument), any future consumer of a
PROV-O-typed provenance graph.

## Competence

**Capabilities:**
- create a fresh `concept_prov_activity` node;
- attach `actor` via role `rrel_actor` and `input` via role `rrel_input`;
- link `input` via `nrel_prov_used` (`PROV:used(a, in_i)`);
- link `provAgent` via `nrel_prov_was_associated_with`
  (`wasAssociatedWith(a, agent_of(s_a))`);
- optionally attach `timestamp` via role `rrel_timestamp` when supplied
  (last argument, checked via `IsElement` — same optional-tail-argument
  convention [[AgentBuildExplanation]] established for
  `rrel_construction_time`);
- finish with an error only when a required argument is missing.

**Limitations (per code):**
- **`agent_of(s_a)` is not computed** — the rule names a mapping
  function from the acting sc-agent to its PROV-agent representation,
  but that function is nowhere formalized in the ontology. `actor` (for
  `rrel_actor`) and `provAgent` (for `wasAssociatedWith`) are separate
  caller-supplied arguments rather than one derived from the other.
- **domain observation, not fixed here:** `rrel_actor` is declared
  `concept_interaction_act → concept_actor`
  (`relations/role_relations.scs`), but the reified activity `a` here
  is a `concept_prov_activity`, not necessarily a
  `concept_interaction_act` — the same class of domain tension already
  documented for [[AgentValidateProvenanceChain]]'s use of
  `nrel_derived_from`.
- **does not fire automatically on process execution** — see the note
  above.
- only `IsElement` on the three required arguments; `timestamp` is
  optional by design, not merely unchecked.

## Input

**Trigger:** arc `action_initiated -> <instance of action_record_prov_activity>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `actor` | `ScAddr` of s_a, the acting entity (role target for `rrel_actor`) | required | only `IsElement` |
| 2 | `input` | `ScAddr` of in_i, a `concept_prov_entity` (role target for `rrel_input`, target of `PROV:used`) | required | only `IsElement` |
| 3 | `prov_agent` | `ScAddr` of the `concept_prov_agent` corresponding to `actor` | required | only `IsElement` |
| 4 | `timestamp` | `ScAddr` of an sc-link for `rrel_timestamp` | optional | `IsElement` gates whether the role is attached |

## Output

A new `concept_prov_activity` node with `rrel_actor`/`rrel_input`
(and `rrel_timestamp` when supplied) roles, `nrel_prov_used` pointing at
`input`, and `nrel_prov_was_associated_with` pointing at `prov_agent`.
Success — `FinishSuccessfully()` + `FormResult(activity, input)`.

## Links

**Dependencies:** `common/relation_io.hpp` (`LinkByRelation`,
`LinkByRole`), `keynodes/provenance_keynodes.hpp` (`ProvenanceKeynodes`,
extended alongside [[AgentRecordProvEntity]] — see its own Links
section for the full list).

**Reads:** nothing beyond argument identity.

**Writes:** one `concept_prov_activity` node + two role arcs (three if
`timestamp` is supplied) + two relation arcs (`nrel_prov_used`,
`nrel_prov_was_associated_with`).

**Workflow.** Data-independent from every other agent. Typically
precedes [[AgentRecordProvEntity]] in a caller's pipeline (the activity
this agent creates becomes that agent's `activity` argument), but
neither agent calls the other directly.

## Behavior

```
IF  actor is not an sc-element
 OR input is not an sc-element
 OR prov_agent is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  activity := GenerateNode(ConstNode) classified concept_prov_activity
  LinkByRole(activity, actor, rrel_actor)
  LinkByRole(activity, input, rrel_input)
  LinkByRelation(activity, input, nrel_prov_used)
  LinkByRelation(activity, prov_agent, nrel_prov_was_associated_with)

  IF timestamp is an sc-element:
    LinkByRole(activity, timestamp, rrel_timestamp)

  action.FormResult(activity, input)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing required
argument; a missing `timestamp` is a normal, non-error outcome (the
role is simply not attached).

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — pure construction
  from caller-supplied identities; a malicious caller could attribute an
  activity to an arbitrary `actor`/`prov_agent`, same caveat as
  [[AgentRecordProvEntity]].
- No PII involved — the data is structural (actor/input/agent identity).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — a handful of connector generations regardless of whether `timestamp` is supplied. |
| Accuracy | Not applicable — deterministic construction; verified by tests covering both the with- and without-timestamp cases. |
| Cost | Negligible. |
| Alerts | None. |

## Tests

File: `tests/test_agent_record_prov_activity.cpp` (3 tests, all pass).

**1. `RecordsActivityWithAllRolesAndTimestamp`**
Input: actor, input, provAgent, and a timestamp link, all supplied.
Output: a new `concept_prov_activity` with all three roles set and both
relation arcs (`nrel_prov_used`, `nrel_prov_was_associated_with`)
pointing correctly; `FormResult` includes the activity and the input.

**2. `OmitsTimestampRoleWhenArgumentMissing`**
Input: only the three required arguments.
Output: `rrel_actor` still set; `rrel_timestamp` absent (not an error).

**3. `FinishesWithErrorOnMissingRequiredArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
