---
type: agent
tags: [agent, sc-agent, provenance, prov-o, explanation]
action_class: action_link_explanation_to_provenance
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentLinkExplanationToProvenance

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_link_explanation_to_provenance`,
corrected 2026-09-02 from blocked to closable) · integration agent wiring
[[AgentBuildExplanation]]'s role data through [[AgentRecordProvEntity]]/[[AgentRecordProvActivity]]'s
PROV-O relations

> [!note] Wiring, not ontology design
> Unlike [[AgentChooseModality]]/[[AgentEvaluateContextAwareLayout]],
> this agent needed zero new ontology — `nrel_prov_was_derived_from`,
> `nrel_prov_used`, `nrel_prov_was_attributed_to`, `rrel_uses_results`
> and `rrel_applies_rules` all already existed. The rule was blocked
> only because nothing had yet created the PROV-O nodes to wire
> together — a gap [[AgentRecordProvEntity]]/[[AgentRecordProvActivity]]
> (2026-09-02, the day before) closed as a side effect of unrelated
> work. See the file breakdown for `rules_provenance_completion.scs` in
> [[Implementation Backlog]] for the corrected entry.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_link_explanation_to_provenance` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentLinkExplanationToProvenance : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_link_explanation_to_provenance.{hpp,cpp}` |
| Role | Implements `rule_link_explanation_to_provenance` (`agent_provenance_registrar` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_provenance_completion.scs` → `rule_link_explanation_to_provenance` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `rule_link_explanation_to_provenance` asks for three PROV-O
facts about every explanation-construction act: `wasDerivedFrom(e, r_i)`
for each used result, `PROV:used(activity_of_e, rule_i)` for each
applied rule, `wasAttributedTo(e, agent_explanation_builder)`. The
relations themselves (`nrel_prov_was_derived_from`, `nrel_prov_used`,
`nrel_prov_was_attributed_to`) were fully typed in
`concepts/provenance.scs` from the start, and the role data they read
from (`rrel_uses_results`, `rrel_applies_rules`) has existed on every
`concept_explanation_construction_act` since [[AgentBuildExplanation]]
— but nothing wired the two together, and
[[AgentBuildExplanation]]'s own Limitations explicitly declined to,
citing an unestablished `concept_prov_entity` domain for
explanation/rule/result nodes.

**Goal.** Given an already-reified construction act and three
caller-supplied PROV-O nodes (the explanation's own entity, its
construction activity, and the builder's PROV agent), write the three
relations the rule names, reading `use_results`/`applies_rules`
directly off the act.

**Value.** Closes the last open item of `rules_provenance_completion.scs`
that had an identifiable, non-open-ended path to closure — `6 of 6`
rules now have either a full or structural-core implementation (see
[[Implementation Backlog]]'s corrected file breakdown).

**Stakeholders:** whatever future π_data computation
([[AgentComputePiMetric]]) or provenance-chain walk
([[AgentValidateProvenanceChain]]) wants explanation acts to
participate in the trace graph, not just results.

## Competence

**Capabilities:**
- for every target of `rrel_uses_results` on `construction_act`, write
  `nrel_prov_was_derived_from(explanation_entity, target)`;
- for every target of `rrel_applies_rules` on `construction_act`, write
  `nrel_prov_used(explanation_activity, target)`;
- write `nrel_prov_was_attributed_to(explanation_entity, builder_prov_agent)`
  once, unconditionally;
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not type `r_i`/`rule_i` as `concept_prov_entity`** —
  `nrel_prov_was_derived_from`/`nrel_prov_used`'s formal domain
  (`concepts/provenance.scs`) calls for it on the second/first side
  respectively, but the rule's own text names `r_i`/`rule_i` directly
  as the relation's target, not a derived PROV wrapper — the same
  "trust the caller for typing" stance every agent in this module takes
  for role-target classes, applied here instead of inventing an
  additional wrapper-entity layer;
- **does not create or type `explanation_entity`/`explanation_activity`/
  `builder_prov_agent`** — all three are caller-supplied, the same
  "agent_of()/activity_of() not computed" stance
  [[AgentRecordProvActivity]] already took for `prov_agent`;
- **no automatic trigger on construction-act appearance** — same
  explicit-action-only convention as every other agent in this module;
- only `IsElement` on all four arguments; none of their classes is
  verified.

## Input

**Trigger:** arc `action_initiated -> <instance of action_link_explanation_to_provenance>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `construction_act` | `ScAddr` of a `concept_explanation_construction_act` ([[AgentBuildExplanation]]'s output) | required | only `IsElement` |
| 2 | `explanation_entity` | `ScAddr` of `e` — target of `wasDerivedFrom`/`wasAttributedTo` | required | only `IsElement` |
| 3 | `explanation_activity` | `ScAddr` of `activity_of_e` — source of `PROV:used` | required | only `IsElement` |
| 4 | `builder_prov_agent` | `ScAddr` of the `concept_prov_agent` for `agent_explanation_builder` | required | only `IsElement` |

## Output

Zero or more `nrel_prov_was_derived_from` arcs from `explanation_entity`
(one per `rrel_uses_results` target), zero or more `nrel_prov_used` arcs
from `explanation_activity` (one per `rrel_applies_rules` target), and
exactly one `nrel_prov_was_attributed_to` arc from `explanation_entity`
to `builder_prov_agent`. Success — `FinishSuccessfully()` +
`FormResult(explanation_entity, explanation_activity)`.

## Links

**Dependencies:** `common/relation_io.hpp` (`LinkByRelation`, and the
new `ForEachRoleTarget` — the read-symmetric counterpart to the existing
`LinkByRole` writer, added for this agent since no prior agent needed to
iterate *all* targets of a role from a single source rather than write
one or iterate a group's members), `keynodes/explanation_keynodes.hpp`
(`rrel_uses_results`, `rrel_applies_rules`,
`concept_explanation_construction_act` — all pre-existing, from
[[AgentBuildExplanation]]), `keynodes/provenance_keynodes.hpp`
(`ProvenanceKeynodes`, extended with `nrel_prov_was_derived_from` — the
one PROV-O relation from `concepts/provenance.scs` no earlier agent had
needed — and `action_link_explanation_to_provenance`).

**Reads:** `rrel_uses_results`/`rrel_applies_rules` role targets of
`construction_act`.

**Writes:** `nrel_prov_was_derived_from`/`nrel_prov_used` arcs (count
depends on the act's role data) plus one `nrel_prov_was_attributed_to`
arc.

**Workflow.** Downstream of [[AgentBuildExplanation]] (needs its
construction act) and, for `explanation_entity`/`explanation_activity`
to be meaningfully typed, typically downstream of
[[AgentRecordProvEntity]]/[[AgentRecordProvActivity]] as well — though
this agent does not call or verify any of the three.

## Behavior

```
IF  construction_act is not an sc-element
 OR explanation_entity is not an sc-element
 OR explanation_activity is not an sc-element
 OR builder_prov_agent is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  FOR EACH r_i IN rrel_uses_results(construction_act):
    LinkByRelation(explanation_entity, r_i, nrel_prov_was_derived_from)

  FOR EACH rule_i IN rrel_applies_rules(construction_act):
    LinkByRelation(explanation_activity, rule_i, nrel_prov_used)

  LinkByRelation(explanation_entity, builder_prov_agent, nrel_prov_was_attributed_to)

  action.FormResult(explanation_entity, explanation_activity)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument; a
`construction_act` with no `rrel_uses_results`/`rrel_applies_rules`
targets (e.g. a counterfactual explanation that used `rrel_based_on`
instead) is a normal outcome — those two loops simply write nothing,
`wasAttributedTo` is still written.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — pure relation-wiring
  from caller-supplied identities; a malicious caller could attribute an
  explanation to an arbitrary `builder_prov_agent`, the same caveat
  already documented for [[AgentRecordProvActivity]].
- No PII involved — the data is structural (explanation/result/rule
  identity).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(k) in the number of `uses_results`/`applies_rules` targets. |
| Accuracy | Not applicable — deterministic relation-wiring; verified by tests covering multi-target iteration, the no-role-targets case, and the error case. |
| Cost | Negligible. |
| Alerts | None. |

## Tests

File: `tests/test_agent_link_explanation_to_provenance.cpp` (3 tests, all pass).

**1. `LinksAllUsedResultsAppliedRulesAndAttribution`**
Input: construction act with two `rrel_uses_results` targets and one
`rrel_applies_rules` target.
Output: two `nrel_prov_was_derived_from` arcs (one per result), one
`nrel_prov_used` arc, one `nrel_prov_was_attributed_to` arc — confirms
`ForEachRoleTarget` visits *every* target of a role, not just the first.

**2. `WritesOnlyAttributionWhenRoleTargetsAreAbsent`**
Input: construction act with neither role populated.
Output: no `wasDerivedFrom`/`PROV:used` arcs, but
`wasAttributedTo` is still written — not an error.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
