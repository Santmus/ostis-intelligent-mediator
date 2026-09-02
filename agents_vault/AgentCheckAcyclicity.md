---
type: agent
tags: [agent, sc-agent, decomposition, graph-traversal, integrity]
action_class: action_check_acyclicity
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentCheckAcyclicity

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_check_acyclicity`, READY) ·
first agent implemented for `agent_task_decomposer` / `rules_decomposition.scs`

> [!note] Checks the DAG, does not build it
> The rule reads "after decomposition run acyclic(depends_on)" — this
> agent does not decompose anything itself (`agent_task_decomposer` is
> not implemented; see Limitations). `request` is passed in already
> decomposed via existing `nrel_decomposed_into`/`nrel_subtask_depends_on`
> data; the agent's job is only the graph check that comes after.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_check_acyclicity` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentCheckAcyclicity : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_check_acyclicity.{hpp,cpp}` |
| Role | Implements `rule_check_acyclicity` (`agent_task_decomposer` in `concepts/sc_agents.scs`) |
| Knowledge base rule | `rules/rules_decomposition.scs` → `rule_check_acyclicity` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `concepts/tasks.scs`'s own `definition_dependency_graph`
requires the subtask dependency graph T(q) to be a DAG
("Ограничение acyclic(depends_on), цикл в графе запрещён") — but nothing
in the project checked this after `rule_apply_decomposition_pattern`
populates `nrel_decomposed_into`/`nrel_subtask_depends_on`.

**Goal.** Given a request q, collect T(q) = {σ : decomposed_into(q, σ)}
and determine whether the subgraph of `nrel_subtask_depends_on` reachable
from T(q) contains a cycle.

**Value.** Operationalizes `affirmation_formula_5_decomposition_acyclicity`
(`concepts/integrity.scs`) as an executable check rather than a
declarative statement, and gives `rule_check_acyclicity`'s "create
concept_integrity_violation" a concrete, typed subclass to instantiate.

**Stakeholders:** `agent_task_decomposer`'s eventual
`rule_apply_decomposition_pattern`/`rule_recursive_decomposition`
implementation (would call this after each decomposition step, per the
rule's own trigger condition), `agent_coalition_reconfigurator`'s
`rule_on_constraint_violation` (would react to the violation this agent
creates — not implemented, same unrealized link noted in
[[AgentExcludeFromCoalition]]).

## Competence

**Capabilities:**
- enumerate T(q) via a single forward iterator over `nrel_decomposed_into(request, ·)`;
- detect a cycle in the `nrel_subtask_depends_on` subgraph reachable from
  T(q) via DFS with `onStack`/`visited` sets (`std::unordered_set<ScAddr, ScAddrHashFunc>`)
  — a node revisited while still on the current DFS path is a cycle; a
  node revisited after leaving the path (a converging, non-cyclic DAG
  edge, e.g. two subtasks sharing a common dependency) is not;
- write `1.0`/`0.0` into a `value_link` on `result_level` (same
  convention as [[AgentValidateProvenanceChain]]/[[AgentValidateCardinalityConstraint]]);
- on a cycle, create `concept_acyclicity_constraint_violation` (new,
  formalized together with this agent — see Links), attach it under
  `result_level`, and link it to `request` via `nrel_concerns`;
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not initiate "пересмотр шаблона декомпозиции"** —
  `agent_task_decomposer` itself is not implemented
  (`rule_apply_decomposition_pattern`/`rule_recursive_decomposition` are
  PARTIAL per [[Implementation Backlog]]: they need `sim_T` and a
  template library undefined in the text).
- **checks reachability from T(q), not only edges strictly within T(q)**
  — if `nrel_subtask_depends_on` pointed outside the request's own
  decomposition, the agent would still walk into it. No scenario fixture
  in the project does this (dependency edges are always within one
  decomposition), so this is a documented boundary, not an observed bug.
- **does not verify `request` is a `concept_user_request`** — only
  `IsElement`, same convention as the rest of the module.
- recursion depth in `DfsHasCycle` is bounded only by the size of the
  reachable subgraph — acceptable for the decomposition sizes seen in
  every scenario (single digits to low tens of subtasks), not
  stack-safety-hardened for adversarially large graphs.

## Input

**Trigger:** arc `action_initiated -> <instance of action_check_acyclicity>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `request` | `ScAddr` of the request q whose decomposition is checked | required | only `IsElement` |
| 2 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level`:
`1.0` if the reachable `nrel_subtask_depends_on` subgraph is acyclic
(including the vacuous case of an empty T(q)), `0.0` if a cycle was
found. On `0.0`, a `concept_acyclicity_constraint_violation` node is also
attached under `result_level` and linked to `request` via `nrel_concerns`.
Success — `FinishSuccessfully()` + `FormResult(request, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`LinkByRelation`), `keynodes/task_keynodes.hpp`
(`TaskKeynodes` — `nrel_decomposed_into` newly added, `nrel_subtask_depends_on`
reused from [[AgentDispatchReadySubtask]]/[[AgentPropagateToDependent]]),
`keynodes/integrity_keynodes.hpp` (`IntegrityKeynodes` —
`concept_acyclicity_constraint_violation` newly added alongside
`concept_cardinality_constraint_violation`/`concept_existence_constraint_violation`
from [[AgentValidateCardinalityConstraint]]; `nrel_concerns` reused).

**Reads:** `nrel_decomposed_into` arcs from `request`; `nrel_subtask_depends_on`
arcs transitively reachable from T(q).

**Writes:** on cycle, one `concept_acyclicity_constraint_violation` node
+ its classifying arc + its `nrel_concerns` arc to `request`; the
`value_link` of `result_level`.

**Workflow.** Data-independent from every other agent in the module.
Same DFS-over-`ScAddrHashFunc`-set traversal pattern as
[[AgentValidateProvenanceChain]]'s BFS, but with an explicit
onStack/visited distinction (BFS reachability only needed one visited
set; cycle detection specifically needs to distinguish "on the current
path" from "already fully explored").

## Behavior

```
IF  request is not an sc-element  OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  onStack := {}, visited := {}
  cycleFound := false
  FOR EACH subtask WITH request -[decomposed_into]-> subtask, WHILE NOT cycleFound:
    IF subtask NOT IN visited:
      cycleFound := DfsHasCycle(subtask, onStack, visited)

  valid := NOT cycleFound

  IF NOT valid:
    create violation (ConstNode) classified concept_acyclicity_constraint_violation
    attach violation under result_level
    LinkByRelation(violation, request, nrel_concerns)

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (valid ? 1.0 : 0.0) to value_link
  action.FormResult(request, value_link)
  → action.FinishSuccessfully()

DfsHasCycle(node, onStack, visited):
  onStack.insert(node); visited.insert(node)
  FOR EACH next WITH node -[subtask_depends_on]-> next:
    IF next IN onStack: RETURN true
    IF next NOT IN visited AND DfsHasCycle(next, onStack, visited): RETURN true
  onStack.erase(node)
  RETURN false
```

**Error handling:** the only path to an error is a missing argument; a
request with no subtasks at all is a normal, non-error, valid (`1.0`)
outcome.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — the check is
  read-mostly (only the violation node/arcs are written), and a
  malicious/malformed caller can at most trigger a deep DFS recursion by
  supplying a pathologically large dependency graph (see Limitations).
- No PII involved — the data is structural (request/subtask identity and
  dependency edges).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(V+E) DFS over the `nrel_subtask_depends_on` subgraph reachable from T(q), plus one `CreateIterator5` scan of `nrel_decomposed_into`. |
| Accuracy | Not applicable — deterministic graph check; verified by tests including a linear chain, a converging (diamond) DAG, and an explicit 2-cycle. |
| Cost | Negligible for the decomposition sizes seen in project scenarios. |
| Alerts | None — the violation is a passive, queryable node; nothing consumes it proactively (`rule_on_constraint_violation`'s trust-lowering reaction is not implemented, same as the coalition-reconfiguration triggers in [[AgentExcludeFromCoalition]]). |

## Tests

File: `tests/test_agent_check_acyclicity.cpp` (5 tests, all pass).

**1. `ValidAcyclicDependencyGraph`**
Input: request decomposed into s1→s2→s3 (linear `depends_on` chain).
Output: indicator `1.0`, no violation.

**2. `InvalidWhenDependencyGraphHasCycle`**
Input: s1 depends_on s2, s2 depends_on s1 (2-cycle).
Output: indicator `0.0`; violation created, `nrel_concerns` points to
`request`; the action does not hang (termination is itself part of what
the test checks).

**3. `ValidWhenRequestHasNoSubtasksAtAll`**
Input: request with no `nrel_decomposed_into` arcs at all.
Output: indicator `1.0` — vacuously acyclic, not an error.

**4. `HandlesDiamondDependencyWithoutFalsePositive`**
Input: s2 and s3 both depend on s1; s4 depends on both s2 and s3
(converging paths, no cycle).
Output: indicator `1.0`, no violation — confirms the `onStack`/`visited`
distinction does not mistake a re-visited-but-finished node for a cycle.

**5. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
