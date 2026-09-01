---
type: agent
tags: [agent, sc-agent, provenance, graph-reachability]
action_class: action_validate_provenance_chain
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentValidateProvenanceChain

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_verify_transitive_closure`, READY) ·
duplicate-closing companion to [[AgentValidateIntegration]]

> [!note] Graph traversal, not edge counting
> Every other agent in the module answers its question with a single
> iterator lookup or a bounded count. This is the first one that walks a
> transitive relation (`nrel_derived_from`) to arbitrary depth — the
> `Implementation Backlog` flagged this file's equivalent rule as "a
> fundamentally different task (graph traversal, not edge counting)."
> Cycle safety (a visited-node set) is therefore a real correctness
> requirement here, not defensive boilerplate.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_validate_provenance_chain` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentValidateProvenanceChain : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_validate_provenance_chain.{hpp,cpp}` |
| Role | Implements `rule_verify_transitive_closure` (`agent_provenance_registrar` in `concepts/sc_agents.scs`), duplicated as `rule_validate_provenance_chain` (`agent_integrity_checker`) |
| Knowledge base rule | `rules/rules_provenance_completion.scs` → `rule_verify_transitive_closure` **and** `rules/rules_integrity_validation.scs` → `rule_validate_provenance_chain` (same check, two files) |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** Both rules ask the same question in different files: does
`nrel_derived_from`, followed transitively from an integrated result,
eventually reach the originating `concept_user_request`? Without an
answer, nothing distinguishes a fully traceable result from one whose
provenance chain silently stops partway.

**Goal.** Given `integrated_result`, perform a breadth-first search over
outgoing `nrel_derived_from` arcs and report whether any reached node is
classified `concept_user_request`.

**Value.** Closes both `rule_verify_transitive_closure` and
`rule_validate_provenance_chain` at once — the same "one agent, two
duplicate rules" move as [[AgentValidateIntegration]] for
`rule_validate_integration`/`rule_validate_integrated_result_shape`.

**Stakeholders:** the not-yet-implemented `rule_fill_missing_provenance`
trigger and `agent_provenance_registrar` escalation path, which per both
rules' text should react to an incomplete chain; auditors verifying that
a published result is fully traceable to the request that caused it.

## Competence

**Capabilities:**
- breadth-first traversal of `nrel_derived_from` from `integrated_result`,
  with a visited-node set (`std::unordered_set<ScAddr, ScAddrHashFunc>`)
  to guarantee termination even if the underlying graph has a cycle —
  the relation is declared transitive but not antireflexive
  (`relations/relations_provenance.scs`), so a cycle is not ruled out by
  the ontology itself;
- report `1.0`/`0.0` via a `value_link` on `result_level`, same
  convention as [[AgentValidateIntegration]];
- on `invalid`, create a `concept_existence_constraint_violation`,
  attach it under `result_level`, and link it to `integrated_result` via
  `nrel_concerns` — same convention as
  [[AgentComputePiMetric]]/[[AgentValidateCardinalityConstraint]];
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not lower π_proc/π_data** — neither rule's text names which
  metric-level node to update, and the agent has no way to know it; a
  separate [[AgentComputePiMetric]] call against the right group is
  still required.
- **does not trigger `rule_fill_missing_provenance`** — an unimplemented
  heuristic without an algorithm, same limitation already documented on
  [[AgentComputePiMetric]].
- **does not escalate to `agent_provenance_registrar`** — not implemented
  as a receiver, same class of gap as
  [[AgentDetectAnomalousRequestRate]] not activating
  `agent_coalition_reconfigurator`.
- **reads a relation nothing in the project currently writes** —
  `nrel_derived_from` appears only in `axioms/axioms_provenance.scs`'s
  prose (`affirmation_derived_from_chain`), never in a scenario fixture,
  and no agent in the codebase creates it. In practice this agent will
  report `invalid` for any real (non-test) `integrated_result` until
  something populates the chain.
- **a documented, not fixed, domain tension** — `nrel_derived_from` is
  declared `concept_processing_result -> concept_processing_result`
  (`relations/relations_provenance.scs`), but `concept_user_request` is a
  `concept_interaction_entity`, not a `concept_processing_result`
  (`concepts/interactions.scs`). The chain this rule asks for formally
  ends outside the relation's declared domain. The agent only reads
  existing arcs and does not create the boundary-crossing one itself, so
  this is left as an observation for whoever eventually writes
  `nrel_derived_from` end-to-end, not resolved here.
- as with the rest of the module — only `IsElement` on the two arguments,
  no semantic validation.

## Input

**Trigger:** arc `action_initiated -> <instance of action_validate_provenance_chain>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `integrated_result` | `ScAddr` of the traversal's start node | required | only `IsElement` |
| 2 | `result_level` | `ScAddr` of the node under which the indicator (and, on failure, the violation) is placed | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level` with
`1.0` (chain reaches a `concept_user_request`) or `0.0` (it does not). If
invalid — additionally a new `concept_existence_constraint_violation`
attached under `result_level` and linked to `integrated_result` via
`nrel_concerns`. Success — `FinishSuccessfully()` +
`FormResult(integratedResult, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`LinkByRelation`), `keynodes/provenance_keynodes.hpp`
(`ProvenanceKeynodes` — extended with `nrel_derived_from`,
`concept_user_request`, `action_validate_provenance_chain`),
`keynodes/integrity_keynodes.hpp` (`IntegrityKeynodes::concept_existence_constraint_violation`,
`nrel_concerns`, already shared with [[AgentComputePiMetric]]).

**Reads:** outgoing `nrel_derived_from` arcs, transitively, from
`integrated_result`; classification of each reached node against
`concept_user_request`.

**Writes:** the `value_link` of `result_level`; on `invalid`, a new
violation node and its `nrel_concerns` link.

**Workflow.** Reads a relation ([[AgentBuildExplanation]]'s
`applies_rules_group`/`uses_results_group` members are unrelated data —
no agent in the project writes `nrel_derived_from`). Not wired to
[[AgentComputePiMetric]] or any escalation path.

## Behavior

```
IF  integrated_result is not an sc-element  OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  visited := {integrated_result}
  frontier := queue([integrated_result])
  reached_request := false

  WHILE frontier is not empty AND NOT reached_request:
    current := frontier.pop()
    FOR EACH next IN outgoing nrel_derived_from targets of current:
      IF  next is classified concept_user_request
      THEN  reached_request := true; BREAK
      ELSE IF  next NOT IN visited
      THEN  visited.add(next); frontier.push(next)

  valid := reached_request

  IF  NOT valid
  THEN
    v := create a node of class concept_existence_constraint_violation
    attach v under result_level (ConstPermPosArc)
    link v -[nrel_concerns]-> integrated_result

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (valid ? 1.0 : 0.0) to value_link
  action.FormResult(integrated_result, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument; an
incomplete or absent chain is a normal, non-error outcome (`valid = false`
plus a violation).

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous. The traversal itself terminates deterministically
because the visited-node set bounds it to the size of the reachable
subgraph, regardless of cycles.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** the traversal has no depth or node-count limit
  beyond the graph's own size — a very large or densely connected
  `derived_from` subgraph would make this call proportionally expensive.
  Nothing in the rule text specifies a bound (unlike, say, `d_max` for
  replanning), so none is invented here.
- The chain itself carries no PII; it is a structural reachability check
  over existing provenance links.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(V+E) over the reachable `derived_from` subgraph — one `CreateIterator5` scan per visited node, each node visited at most once. |
| Accuracy | Not applicable — deterministic graph reachability; verified by tests including a cyclic graph. |
| Cost | Negligible for the small chains exercised by tests; scales with the size of the actual provenance graph, which no agent currently populates. |
| Alerts | None — the indicator and violation are passive, queryable signals; nothing consumes them proactively. |

## Tests

File: `tests/test_agent_validate_provenance_chain.cpp` (5 tests, all pass).

**1. `ValidatesSuccessfullyWhenChainReachesUserRequest`**
Input: `ir -[derived_from]-> r1 -[derived_from]-> request` (`request`
classified `concept_user_request`), a two-hop chain.
Output: indicator `1.0`, no violation.

**2. `InvalidWhenChainDoesNotReachRequest`**
Input: `ir -[derived_from]-> r1`, `r1` not classified `concept_user_request`,
chain ends there.
Output: indicator `0.0`, violation created and linked to `ir` via
`nrel_concerns`.

**3. `InvalidWhenNoOutgoingArcsAtAll`**
Input: `ir` with no outgoing `derived_from` arcs at all.
Output: indicator `0.0`, violation created — no chain is the same failure
mode as an incomplete one.

**4. `HandlesCycleWithoutHangingAndReportsInvalid`**
Input: `ir -[derived_from]-> r1 -[derived_from]-> r2 -[derived_from]-> r1`
(a cycle between `r1` and `r2`), no `concept_user_request` anywhere.
Output: the action still completes within the test's 2-second wait
(proving the visited-set termination guard actually works) and reports
indicator `0.0`.

**5. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
