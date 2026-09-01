---
type: agent
tags: [agent, sc-agent, explanation, provenance]
action_class: action_build_explanation
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentBuildExplanation

← [[Agent Index]] · from [[Implementation Backlog]] (`agent_explanation_builder`) ·
first agent in the explanation domain (AGENTS_AUDIT.md §7.5)

> [!note] One agent — four rules
> `rules/rules_explanation.scs` describes four separate rules
> (`rule_build_local_explanation`, `_global`, `_contrastive`, `_counterfactual`).
> Like [[AgentComputePiMetric]] for π_proc/π_dec/π_data, this agent
> implements their common **structural** core once — reification of
> `concept_explanation_construction_act` per `prov_template_explanation`
> — and lets **the caller** pick the concrete explanation class
> (`concept_explanation_local`/`_global`/`_contrastive`/`_counterfactual`/…)
> by argument.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_build_explanation` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentBuildExplanation : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_build_explanation.{hpp,cpp}` |
| Role | Implements the structural part of `rule_build_local_explanation` / `_global` / `_contrastive` / `_counterfactual` (`agent_explanation_builder` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_explanation.scs` → all four `rule_build_*` rules |
| Module | `TrustModule` (not renamed — see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `agent_explanation_builder`'s output must be a traceable
sc-structure (`concept_explanation_construction_act` per
`axioms/axioms_provenance.scs` → `prov_template_explanation`), not just a
bare `concept_explanation` node — otherwise a consumer cannot recover which
result the explanation covers, which type it is, or what it was built from.
[[AgentResolveByTrustWeighted]] already creates a bare `concept_explanation`
node as a structural placeholder for `nrel_resolves_conflict`, explicitly
documented there as *not* filling in this reification — that gap is what
this agent closes.

**Goal.** Given an explanandum and an explanation type, create the
explanation node, link it to the explanandum via the correct provenance
relation(s), and reify the construction act with all five role relations
the template names — using only whatever supporting material
(used results, applied rules, knowledge sources, a timestamp) the caller
actually supplies.

**Value.** A single, reusable reification for all four `rule_build_*`
rules and for [[AgentResolveByTrustWeighted]]'s own future use, instead of
duplicating the same act-construction code four times.

**Stakeholders:** `agent_explanation_builder` callers (currently only
[[AgentResolveByTrustWeighted]] would benefit, not yet wired to call it),
auditors/regulators inspecting `concept_explanation_construction_act`,
XAI-facing consumers of `concept_explanation`.

## Competence

**Capabilities:**
- create a new node, classify it as the caller-supplied `explanation_type`
  class, and link `explanandum -[nrel_supported_by_explanation]->`
  explanation (domain `concept_processing_result`, per
  `relations/relations_provenance.scs`);
- additionally link `explanandum -[nrel_explained_by]-> explanation` when
  `explanandum` is already classified `concept_integrated_result` (domain
  of `nrel_explained_by`) — matching both relations being set together on
  `integrated_result_19` in `scenarios/scenario_explanation_construction.scs`;
- reify a `concept_explanation_construction_act` with role
  `rrel_explanandum` → explanandum and `rrel_explanation_type` →
  explanation_type (always), plus `rrel_uses_results` /
  `rrel_applies_rules` / `rrel_based_on` (one arc per member of the
  corresponding optional group argument) and `rrel_construction_time`
  (if a timestamp sc-link is passed) — exactly the five roles
  `prov_template_explanation` names;
- finish with an error only when `explanandum` or `explanation_type` is
  missing.

**Limitations (per code):**
- **no natural-language content** — the agent builds the typed,
  provenance-linked structure only; it does not write an `nrel_main_idtf`
  description onto the explanation node (contrast with the human-readable
  text on `explanation_collective_002` in the scenario). Generating that
  text remains `agent_explanation_builder`'s un-formalized job.
- **no type-specific derivation** — for a contrastive explanation, the
  caller must compute *what differs* between X and Y and pass it via
  `uses_results_group`/`based_on_group`; for a counterfactual, the caller
  must compute the minimal input change itself. The agent has no algorithm
  for either — it only reifies whatever the caller already worked out.
- **does not set `nrel_prov_used`** — `prov_template_explanation` also
  asks that used rules/sources be added to the provenance graph via
  `PROV:used` (`nrel_prov_used`, domain `concept_prov_activity` →
  `concept_prov_entity`, `concepts/provenance.scs`). Neither the
  explanation node nor `applies_rules_group`/`based_on_group` members are
  formally typed as `concept_prov_activity`/`concept_prov_entity`
  anywhere in the KB; doing so here would mean inventing typing rather
  than reusing an established one — the same class of gap that blocks
  AGENTS_AUDIT.md §7.4. Deferred pending that design decision.
- **does not validate `explanation_type`** — the agent trusts whatever
  class node the caller passes; it never checks that it is actually a
  subclass of `concept_explanation`. Same convention as every other agent
  in the module (caller-supplied classification is trusted).
- **does not check for an existing explanation** — `rule_build_local_explanation`'s
  precondition is `¬supported_by_explanation(r, _)`; the agent does not
  verify this and will happily create a second explanation for an
  `explanandum` that already has one. Enforcing "at most one" is the
  caller's responsibility.
- as with the rest of the module — only `IsElement` is checked on
  `explanandum`/`explanation_type`; no semantic validation.

## Input

**Trigger:** arc `action_initiated -> <instance of action_build_explanation>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `explanandum` | `ScAddr` of a `concept_processing_result` (or subtype, e.g. `concept_integrated_result`) instance | required | only `IsElement` |
| 2 | `explanation_type` | `ScAddr` of a class node (e.g. `concept_explanation_local`) | required | only `IsElement` |
| 3 | `uses_results_group` | `ScAddr` of a group node (members via outgoing `ConstPermPosArc`) | optional | skipped if not an element |
| 4 | `applies_rules_group` | same shape, for applied rules | optional | skipped if not an element |
| 5 | `based_on_group` | same shape, for knowledge sources | optional | skipped if not an element |
| 6 | `construction_time_link` | `ScAddr` of an sc-link holding a timestamp | optional | skipped if not an element |

**Format:** the three group arguments follow the same "membership via
plain `ConstPermPosArc`" convention as `elements_group` in
[[AgentComputePiMetric]] — not sc-links, not `rrel_*`-typed sets.

## Output

A new node classified `explanation_type`, linked from `explanandum` via
`nrel_supported_by_explanation` (and `nrel_explained_by` when applicable),
plus a new `concept_explanation_construction_act` node carrying up to five
role relations (two always, four conditional on the matching optional
argument). Success — `FinishSuccessfully()` +
`FormResult(explanationAddr, constructionActAddr)`.

## Links

**Dependencies:** `common/relation_io.hpp` (`LinkByRelation` for the two
non-role provenance relations, `LinkByRole` for the five role relations —
both already shared helpers, no new ones added),
`keynodes/explanation_keynodes.hpp` (new — `ExplanationKeynodes`).

**Reads:** membership of the three optional group arguments (`ConstPermPosArc`),
classification of `explanandum` as `concept_integrated_result` (`CheckConnector`).

**Writes:** a new explanation node + its classification arc, up to two
provenance relation arcs from `explanandum`, a new construction-act node +
its classification arc, and up to five role arcs from the act.

**Workflow.** Not wired to any other agent yet:
[[AgentResolveByTrustWeighted]] still creates its own bare
`concept_explanation` node inline rather than calling this action — see
its own Limitations. Conceptually the next step in closing that gap, not
done in this pass to keep the two agents' behavior independently
verifiable.

## Behavior

```
IF  explanandum is not an sc-element  OR  explanation_type is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  e := create node, classify e <- explanation_type
  LinkByRelation(explanandum, e, nrel_supported_by_explanation)
  IF  explanandum classified concept_integrated_result
  THEN  LinkByRelation(explanandum, e, nrel_explained_by)

  act := create node, classify act <- concept_explanation_construction_act
  LinkByRole(act, explanandum, rrel_explanandum)
  LinkByRole(act, explanation_type, rrel_explanation_type)
  FOR EACH member IN uses_results_group (if present):
    LinkByRole(act, member, rrel_uses_results)
  FOR EACH member IN applies_rules_group (if present):
    LinkByRole(act, member, rrel_applies_rules)
  FOR EACH member IN based_on_group (if present):
    LinkByRole(act, member, rrel_based_on)
  IF  construction_time_link is an sc-element
  THEN  LinkByRole(act, construction_time_link, rrel_construction_time)

  action.FormResult(e, act)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing `explanandum`
or `explanation_type`; missing/empty optional groups or a missing
timestamp are not errors — the corresponding roles are simply absent from
the act (same "no data = write nothing" convention as
[[AgentComputePiMetric]]).

**Timeouts:** as with the rest of the module's agents — no internal
timeout, `DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the agent trusts `explanation_type` completely — a
  caller can classify an explanation as `concept_explanation_local` while
  actually reifying a global/contrastive act (or an unrelated class
  entirely), and nothing detects the mismatch.
- Explanations are metadata about processing, not primary data; the risk
  surface is misleading/incomplete audit trails rather than PII exposure
  — a caller-supplied `uses_results_group`/`based_on_group` could still
  reference PII-bearing nodes, which this agent does not inspect.
- Creating a second explanation for an already-explained `explanandum`
  (see Limitations) is not itself a security issue, but a consumer that
  assumes uniqueness (e.g. always follows the first arc found) could be
  silently misled about which explanation is authoritative.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; linear in the combined size of the three optional groups — one `CreateIterator3` scan per non-empty group plus O(1) node/arc generation. |
| Accuracy | Not applicable — deterministic structural construction; correctness verified by tests reproducing `scenario_explanation_construction.scs`'s `explanation_77`. |
| Cost | Negligible for typical group sizes. |
| Alerts | None — the created act is a passive, queryable structure; nothing consumes it proactively. |

## Tests

File: `tests/test_agent_build_explanation.cpp` (4 tests, all pass).

**1. `BuildsReifiedConstructionActWithAllRoles`**
Input: reproduces `explanation_77` from `scenario_explanation_construction.scs`
— explanandum (plain result, not integrated), an explanation_type node,
one-member `uses_results`/`applies_rules` groups, a two-member `based_on`
group, and a construction-time link (`2025-09-12T14:09:14Z`).
Output: `nrel_supported_by_explanation` set, `nrel_explained_by` absent
(not an integrated result), construction act found with all five roles
populated exactly as passed; both explanation and act present in
`FormResult`.

**2. `LinksExplainedByOnlyForIntegratedResultAndOmitsMissingRoles`**
Input: explanandum pre-classified `concept_integrated_result`, no optional
arguments.
Output: both `nrel_supported_by_explanation` and `nrel_explained_by` point
to the same explanation node; the act exists with only the two mandatory
roles — `rrel_uses_results`/`_applies_rules`/`_based_on`/`_construction_time`
are all empty, not errors.

**3. `DoesNotCreateRoleLinksForEmptyGroups`**
Input: the three optional group arguments are present but have no members.
Output: `FinishedSuccessfully`, act created, but no role arcs for any of
the three group-backed roles — mirrors [[AgentComputePiMetric]]'s
"nothing to measure ≠ error" convention.

**4. `FinishesWithErrorOnMissingRequiredArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
