---
type: agent
tags: [agent, sc-agent, allocation, admissibility, lifecycle]
action_class: action_allocate_subtasks
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentAllocateSubtasks

← [[Agent Index]] · AGENTS_AUDIT.md §7.5, the agent the audit named as
unblocking both §7.3 and §7.4 · designed through a five-seat commission,
protocol and per-decision justification in
[[AgentAllocateSubtasks Commission]] (ratified by the author 2026-09-13)

> [!note] Admissibility gate and selection — not a match calculator
> This agent does **not** compute `match(σ,s|C)`. Of the six terms in
> `eq:semantic_match`, chapter 2 formalizes only `sim_T` (normalized
> Wu–Palmer, `eq:sim_type` — still absent from the KB, see
> [[Implementation Backlog]]) and `trust` (the `Σλₖtₖ` convolution);
> `sim_In`, `sim_Out`, `qos` and the five `sim_k` inside `ctx_compat`
> remain free functions. Summing a weighted total with four undefined
> terms would produce a number that looks like `match` but isn't — and
> it would enter the assignment act as a recorded fact. The value is
> read from a reified `concept_match_assessment` instead, the same
> "premise supplied by the caller" device already used by
> [[AgentHandleNoAdmissibleAssignment]], [[AgentExcludeFromCoalition]]
> and [[AgentReassignOnResourceExhaustion]].

## Identification

| Field | Value |
|---|---|
| Action class (sc-IRI) | `action_allocate_subtasks` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentAllocateSubtasks : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_allocate_subtasks.{hpp,cpp}` |
| Role | Operationalizes `rule_assignment_admissibility` (`rules/rules_allocation.scs`) and step 2.3 of the greedy allocation listing of chapter 2 |
| KB declaration | `rules/rules_allocation.scs`; OSTIS `abstract_sc_agent` declaration in `concepts/sc_agent_realizations.scs` (27th agent) |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `agent_task_allocator` was the only agent in the catalogue
whose own rules were never implemented: 1 of its 4 rules
([[AgentHandleNoAdmissibleAssignment]]) covered only the *failure*
branch. Nothing in the project ever established that an assignment was
admissible — `nrel_assigned_to` appeared only as static scenario
fixture. [[AgentDispatchReadySubtask]] dispatched on the *presence* of
an assignee it had no way to validate (§7.3), and trust aggregation had
no allocator to consume it (§7.4).

**Goal.** Given a subtask, a threshold, a multiplicity and a context,
establish `S_{σ,C}`, pick the `k(σ)` best-matching admissible executors,
and record each assignment as a fully reified act.

## Arguments (arity 6)

| # | Argument | Why it is an argument |
|---|---|---|
| 1 | `subtask` — σ | |
| 2 | `thresholdLink` — θ, `[0,1]` | θ is declared a *tuning* parameter in chapter 2, fixed before the evaluation run; hard-coding it in C++ would remove it from reproducibility |
| 3 | `multiplicityLink` — k(σ) ≥ 1 | multiplicity follows from route type, but no `concept_route_*` in the KB is attached to any request and no carrier for `k(σ)` exists |
| 4 | `context` — C | becomes `rrel_context` of the act |
| 5 | `assignmentTime` | `prov_template_assignment` requires `rrel_assignment_time`; no agent in this project has a time source (§7.3), so it is supplied, never synthesized |
| 6 | `resultLevel` | indicator link + generated acts |

Result: `1.0` when `|S_{σ,C}| ≥ k(σ)` and the assignment was built;
`0.0` otherwise — the caller's signal to take the step 2.2 path via
[[AgentHandleNoAdmissibleAssignment]].

## What is checked, and on what authority

| Conjunct of `assign(σ,s)` | Checked? | Authority |
|---|---|---|
| `match(σ,s∣C) ≥ θ` | yes, over a **supplied** value | `eq:assignment_admissibility`; value from `concept_match_assessment` / `nrel_match_value` |
| `available(s,C)` | **yes** | registry entry status is `concept_registry_status_active` — `affirmation_registry_required_for_assignment` states assignment is admissible only for a participant with an active entry |
| `permit(view(C,s),s,C)` | **yes** | a policy on the entry (`nrel_entry_policy`) permits the system via `nrel_permits` |
| `valid(C,L_s)` | **no** | no comparison predicate exists in either the text or the KB — `L_s` instances are natural-language links ("requires GPU ≥ 6 GB VRAM"). Named open tail, **not** silently assumed true |

Absence of a fact is never read as permission: an entry with no policy,
or a system with no registry entry, is inadmissible.

## Selection — step 2.3, and why it is exact

`std::stable_sort` by descending `match`, then the first `k(σ)`. Under
`stmt:greedy_optimal` this is not an approximation: the objective is
additive over subtasks and `S_{σ,C}` is determined independently of
other subtasks' assignments, so the componentwise maximum coincides with
the maximum of the sum. `stable_sort` keeps ties in KB enumeration order
rather than leaving them to the sort algorithm.

The binding constraint `consistent(C,χ)` is out of scope — with a
non-empty set of such conditions the problem stops being separable, the
objective is not submodular and no constant-factor guarantee holds. This
agent works over one subtask, where the constraint holds identically.

## What it produces

Per selected executor:
- `nrel_assigned_to(σ,s)` — deliberately non-functional, since `χ` maps
  into subsets and `|χ(σ)| = k(σ)`;
- an act of `concept_process_allocation` with `rrel_subtask`,
  `rrel_assignee`, `rrel_context`, `rrel_assignment_time`,
  `rrel_required_confidence` (= θ), `rrel_initiator`
  (= `agent_task_allocator`) and `rrel_priority` — the last **carried
  over** from the subtask's `nrel_subtask_priority`, and omitted
  entirely if the subtask has none.

No separate `concept_provenance_record` is attached. `nrel_has_provenance`
is declared over `concept_processing_result ∪ concept_audit_record`
(`relations/relations_provenance.scs`) and an allocation act is neither;
`prov_template_assignment` says the act's own reification *is* its
provenance. Hanging provenance here would break an already-fixed domain
— precisely what §7.4 declined to do.

## Ontology actualized alongside (ratified, В-1…В-4)

`assign(σ,s)` had been recorded in the KB in **five divergent editions**
— the commission found three, an exhaustive second sweep found two more.
Only `concepts/capabilities.scs` carried the current four-conjunct form.
Brought into line: `constraint_assignment_admissibility`
(`axioms/axioms_constraints.scs`), `rule_assignment_admissibility` and
the premise of `rule_handle_no_admissible_assignment`
(`rules/rules_allocation.scs`), the `S_q` formula (`concepts/tasks.scs`,
which used an `allow(q,P_k)` predicate defined nowhere), and
`affirmation_capability_match` (`definitions/definitions_model.scs`,
which additionally folded the argmax into the admissibility condition —
chapter 2 keeps admissibility and selection apart).

A second family of stale statements was found in the same sweep: the
**functional form `χ : T(q) → S`** in four places, where
`eq:assignment_map` requires `χ : T(q) → P(S) ∖ {∅}` with
`|χ(σ)| = k(σ)` — and chapter 2 argues explicitly that the functional
form would rule out both the parallel route and coalition formation,
which is why `assignedTo` is not declared functional. That edition
contradicted *this very agent*, which selects top-`k`. Also corrected:
`rule_on_system_failure` picked a replacement executor "by `match ≥ θ`",
whereas step 2.5 requires choosing from the admissible set `S_{σ,C}`. `tests/tests_capability_match.scs` was rebased onto chapter 2's
parameters (θ = 0.70, `w_R` = 0.25, remaining five 0.15); TM1 recomputes
exactly to 0.928, while TM2–TM4 record no component breakdown and so are
explicitly marked non-recomputable rather than back-filled.

New KB elements: `concept_match_assessment`, `nrel_match_value`,
`action_allocate_subtasks` (§5.2 collision grep: zero hits for each).

## Tests

`tests/test_agent_allocate_subtasks.cpp` — 10 tests, including
`ReproducesChapterTwoWorkedExample`, which replays the dissertation's own
allocation table (s₁ = 0.895, s₂ = 0.828, s₄ = 0.608 at θ = 0.70,
k = 2) and asserts that s₄ is cut off exactly as the text says.
Verification per methodology §5.3: `ctest` 116/116, `sc-builder --clear`
clean at 34975 elements (baseline before this work: 34741), production
`repo/` untouched.

## Not implemented

- re-decomposition when `|S_{σ,C}| < k(σ)` (step 2.2a) —
  `rule_apply_decomposition_pattern` is itself unimplemented;
- reassignment against the `b_q` budget (step 2.5) — that is
  `AgentReassignSubtask`'s area (§7.5);
- `valid(C,L_s)`, `consistent(C,χ)`, and the computation of `match`
  itself — see above;
- `sim_T` by Wu–Palmer — ratified as the next backlog item
  (`AgentComputeSimType`), the only `match` term formalized well enough
  to implement without inventing anything.
