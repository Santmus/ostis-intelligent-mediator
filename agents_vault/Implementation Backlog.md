---
type: backlog
tags: [backlog, agents, survey]
status: survey only, implementation not started
surveyed: 2026-08-18
---

# Implementation Backlog

← [[Agent Index]]

Review of `ostis_kb_user_ecosystem/` to identify which rules/agents can still
be operationalized as C++ sc-agents — by analogy with the already implemented
[[Trust Model|agent_trust_evaluator]] (5 agents, `rules_trust_update.scs`).

> [!note] Update 2026-08-20
> The entire top-5 is implemented: [[AgentComputePiMetric]]
> (`rule_compute_pi_metrics`), [[AgentDetectAnomalousRequestRate]]
> (`rule_detect_anomalous_request_rate`), [[AgentResolveByTrustWeighted]]
> (`rule_resolve_by_trust_weighted`), [[AgentDispatchReadySubtask]]
> (`rule_dispatch_ready_subtask`) and [[AgentValidateCardinalityConstraint]]
> (core of six SHACL shapes in `rules_integrity_validation.scs`).
> The remaining items in this document are still survey only, implementation not started.

> [!note] Update 2026-08-25, resolved 2026-08-30
> Chapter 2 was revised on 2026-08-24, after the last agent commit. The already
> implemented trust agents diverged from it on two points (default weights
> `λ_k = 1/4`; α/β/γ/δ consolidated into `η`). Both applied 2026-08-30.
> The unimplemented
> rules surveyed below are unaffected.

> [!note] Update 2026-08-22
> `rules_cooperation.scs` is fully covered: [[AgentRegisterPartialResult]]
> (`rule_register_partial_result`) and [[AgentPropagateToDependent]]
> (`rule_propagate_to_dependent`) are implemented after
> [[AgentDispatchReadySubtask]] — `agent_cooperative_executor` is now the
> first agent in the catalog implemented at 100% (3 of 3 rules).
> The remaining items in this document are still survey only.

> [!note] Update 2026-09-01
> `rules_explanation.scs` moves from "not suitable now" to partial:
> [[AgentBuildExplanation]] implements the structural core shared by all
> four `rule_build_*` rules (reification of
> `concept_explanation_construction_act` per `prov_template_explanation`),
> leaving only the per-type semantic content (aggregation for global,
> difference-finding for contrastive, minimal-change search for
> counterfactual) genuinely undone — see its Limitations. `rule_validate_integration`
> (`rules_integration.scs`) is now also implemented, [[AgentValidateIntegration]] —
> it depends on exactly the `nrel_explained_by` data [[AgentBuildExplanation]]
> produces, so the two land together. `rule_verify_transitive_closure`
> (`rules_provenance_completion.scs`, duplicated as
> `rule_validate_provenance_chain` in `rules_integrity_validation.scs`) is
> also now implemented, [[AgentValidateProvenanceChain]] — a BFS over
> `nrel_derived_from` to a `concept_user_request`, the first agent in the
> module to walk a transitive relation rather than check a single arc.
> `rules_coalition_reconfiguration.scs`'s two READY rules
> (`rule_on_system_failure`, `rule_on_resource_exhaustion`) are also now
> implemented, [[AgentExcludeFromCoalition]] and
> [[AgentReassignOnResourceExhaustion]] — the mechanical state-transition
> half of each; the `rule_coalition_optimization` recalculation neither
> triggers remains the same unsolved combinatorial problem blocking
> `rules_allocation.scs`.

> [!note] Update 2026-09-02
> `rule_check_acyclicity` (`rules_decomposition.scs`) is now implemented,
> [[AgentCheckAcyclicity]] — DFS cycle detection over
> `nrel_subtask_depends_on` restricted to T(q) = decomposed_into(q, ·),
> the first agent for `agent_task_decomposer`. The other two rules in
> that file (`rule_apply_decomposition_pattern`,
> `rule_recursive_decomposition`) remain PARTIAL — both need `sim_T` and
> a template library not defined in the text, same class of gap as
> `rule_coalition_optimization`. `rule_right_to_explanation`
> (`rules_security_privacy.scs`) is also now implemented,
> [[AgentValidateRightToExplanation]] — the second agent for F_rule_sec
> alongside [[AgentDetectAnomalousRequestRate]]; reuses `nrel_explained_by`
> (the same relation [[AgentBuildExplanation]] writes and
> [[AgentValidateIntegration]] checks) rather than the differently-named
> `nrel_supported_by_explanation` the neighboring axiom uses for a
> similar obligation — see the agent's own Limitations for that
> terminology mismatch. `rule_progressive_disclosure`
> (`rules_user_adaptation.scs`) is also now implemented,
> [[AgentEvaluateProgressiveDisclosure]] — the first agent for
> F_rule_adapt: a two-threshold decision (provenance-chain depth via
> `nrel_derived_from`, partial-result count via `elements_group`-style
> membership). Its sibling `rule_choose_modality`, previously labeled
> READY below ("inputs are Boolean/class flags already existing as
> sc-facts"), turned out on inspection NOT to be — see the corrected
> entry in the file breakdown below.

> [!note] Update 2026-09-02 (continued)
> `rule_choose_modality` is now also implemented, [[AgentChooseModality]] —
> after the readiness correction above, two narrow ontology subclasses
> were added first (`concept_visual_perception_constraint`
> `<= nrel_inclusion: concept_user_situation_context`,
> `concepts/contexts.scs`; `concept_preference_visual_modality`
> `<= nrel_inclusion: concept_user_preference`, `concepts/users.scs`),
> naming exactly what the rule's own prose already described.
> `rules_user_adaptation.scs` is now 2 of 6 rules implemented.

> [!note] Update 2026-09-02 (continued further)
> `rule_auto_record_prov_entity` and `rule_auto_record_prov_activity`
> (`rules_provenance_completion.scs`) are now implemented,
> [[AgentRecordProvEntity]] and [[AgentRecordProvActivity]] — populating
> the PROV-O triad (`concept_prov_entity`/`_activity`/`_agent`,
> `concepts/provenance.scs`) that had been fully typed but never
> written to. Unlike `rule_choose_modality`, these two rules were
> genuinely READY as originally labeled — no ontology gaps found.
> `rules_provenance_completion.scs` is now 4 of 6 rules implemented,
> and `rule_link_explanation_to_provenance`'s specific blocker (missing
> typed PROV-O inputs) is removed, though the rule itself remains
> unimplemented — see its corrected entry below.

> [!note] Update 2026-09-02 (continued further still)
> `rule_handle_no_admissible_assignment` (`rules_allocation.scs`) is now
> implemented, [[AgentHandleNoAdmissibleAssignment]] — the first agent
> for `agent_task_allocator`, and (per this survey) the last rule in the
> entire backlog that was genuinely READY and still unimplemented.
> Everything remaining below is PARTIAL (blocked on an undefined
> formula/table) or NO (LLM-shaped). The rule's own premise
> (`match(σ,s)` undefined for 5 of 6 terms) is trusted from the caller,
> not recomputed; the mechanical consequent — decomposition path vs.
> integrity violation — is resolved via
> `affirmation_subtask_executability`'s composite/atomic split.

The readiness benchmark is the already implemented pattern: 2–5 numeric
sc-links as text → explicit formula → writing the result back to sc-memory
(see [[AgentUpdateTrustReliability]] and others).

## Agent Catalog (`concepts/sc_agents.scs`)

`concept_sc_agent` (⊆ `concept_intelligent_agent`) is partitioned into
**12 agents**. The first 8 have `nrel_implements_process`/
`nrel_realizes_mapping` specified; the last 4 only have
`nrel_primary_condition`/`nrel_agent_input`/`nrel_agent_output`
(less formally specified). `agent_context_updater` and
`agent_conflict_detector` are not included in the `rrel_key_sc_element`
list of the section — an internal inconsistency of the KB catalog itself.

| Agent sys-idtf | Purpose | Rules file | Status |
|---|---|---|---|
| `agent_request_interpreter` | request interpretation ι: Q_ext×C → Q_form | `rules_interpretation.scs` | not implemented |
| `agent_task_decomposer` | request decomposition δ: Q_form → 2^T | `rules_decomposition.scs` | **partial (1 of 3 rules — [[AgentCheckAcyclicity]])** |
| `agent_task_allocator` | subtask allocation via match(σ,s) | `rules_allocation.scs` | **partial (1 of 4 rules — [[AgentHandleNoAdmissibleAssignment]])** |
| `agent_cooperative_executor` | cooperative execution (FIPA-ACL) | `rules_cooperation.scs` | **✅ implemented (3 agents)** |
| `agent_result_integrator` | result integration ρ: 2^Y×C → Y_int | `rules_integration.scs` | **partial (2 of 5 rules — [[AgentResolveByTrustWeighted]], [[AgentValidateIntegration]])** |
| `agent_explanation_builder` | building explanations/provenance chains | `rules_explanation.scs` | **partial (structural core of 4 rules — [[AgentBuildExplanation]])** |
| **`agent_trust_evaluator`** | **trust vector update T(s,τ)** | `rules_trust_update.scs` | **✅ implemented (5 agents)** |
| `agent_coalition_reconfigurator` | compensatory coalition recalculation | `rules_coalition_reconfiguration.scs` | **partial (2 of 4 rules — [[AgentExcludeFromCoalition]], [[AgentReassignOnResourceExhaustion]])** |
| `agent_integrity_checker` | integrity checking (SHACL-like shapes) | `rules_integrity_validation.scs` | **✅ implemented (7 of 7 rules — [[AgentValidateCardinalityConstraint]] ×6, [[AgentValidateProvenanceChain]] ×1)** |
| `agent_provenance_registrar` | provenance link registration | `rules_provenance_completion.scs` | **partial (4 of 6 rules — [[AgentComputePiMetric]], [[AgentValidateProvenanceChain]], [[AgentRecordProvEntity]], [[AgentRecordProvActivity]])** |
| `agent_context_updater` | interaction context update | — (no separate file) | not implemented |
| `agent_conflict_detector` | result conflict detection | `rules_integration.scs` (partial) | not implemented |

`rules_security_privacy.scs` and `rules_user_adaptation.scs` do not have a
separately assigned "owner" agent in the catalog — they are collectively
attached to "security sc-agents" and to the adaptive interface +
`agent_explanation_builder`.

## Readiness Legend

- **READY** — explicit formula/algorithm, inputs/outputs are clear, can be implemented almost as-is.
- **PARTIAL** — numeric/structural part exists, but depends on quantities or subfunctions not defined in the KB text (additional design required).
- **NO** — purely declarative/philosophical text, nothing to compute (or the task is essentially NLP/heuristics without a formula).

## File-by-File Breakdown

### `rules_allocation.scs` — agent `agent_task_allocator`, 4 rules — **partial (1 of 4 rules)**

- `rule_compute_match_function` — `match(σ,s)=w_T·sim_T+w_I·sim_I+w_O·sim_O+w_C·ctx_compat+w_Q·qos_score+w_R·trust(s,τ,h)` — **PARTIAL**
- `rule_assignment_admissibility` — `assign(σ,s) ← match(σ,s)≥θ ∧ valid(C,L_s)` — **PARTIAL**
- `rule_coalition_optimization` — `χ*=arg max Σ match(σ,χ(σ))` under coverage∧consistent — **PARTIAL**
- `rule_handle_no_admissible_assignment` — IF no admissible s THEN recursive decomposition/integrity_violation — **READY** → ✅ mechanical part implemented, [[AgentHandleNoAdmissibleAssignment]] (the "no admissible executor" premise itself is trusted from the caller, not recomputed — match() remains undefined; see below)

The match() formula is explicit, but of the 6 terms only the `trust(s,τ,h)`
term is ready (already implemented via [[AgentAggregateTrust]]) — the other five
(sim_T, sim_I, sim_O, ctx_compat, qos_score) are not defined in the text.
`rule_coalition_optimization` is a combinatorial assignment problem without
a specified solution algorithm. `rule_handle_no_admissible_assignment`'s own
premise depends on the same undefined `match()`, but its *consequent* — decide
between the decomposition path and an integrity violation, via
`affirmation_subtask_executability`'s composite/atomic split
(`concepts/tasks.scs`) — is fully mechanical once the premise is taken as
given, the same "trust the caller for the triggering fact" pattern already
used for `rule_on_system_failure`/`rule_on_resource_exhaustion`.
Numeric example: `scenario_request_processing.scs` —
`rrel_required_confidence: [0.85]` (threshold θ), but the match() value itself
is nowhere computed explicitly.

### `rules_coalition_reconfiguration.scs` — agent `agent_coalition_reconfigurator`, 4 rules

- `rule_on_system_failure` — IF status=failed∧assigned_to∧member_of THEN exclude s, recalculate coalition — **READY** → ✅ mechanical part implemented, [[AgentExcludeFromCoalition]] (recalculation itself still not done)
- `rule_on_resource_exhaustion` — on resource_exhausted mark reassigned, recalculate — **READY** → ✅ mechanical part implemented, [[AgentReassignOnResourceExhaustion]] (recalculation itself still not done)
- `rule_on_constraint_violation` — "lower trust(s,τ,h)" without a formula for the amount of lowering — **PARTIAL**
- `rule_on_context_drift` — `ctx_compat(C',s) < θ_ctx` → exclude s — **PARTIAL** (ctx_compat not defined)

The two trigger rules (failure/resource exhaustion) are pure state-transition
logic — both now implemented, minus the `rule_coalition_optimization`
recalculation step neither can do (see `rules_allocation.scs` below: it's
the same unsolved combinatorial assignment problem). The other two rules
depend on undefined quantities.
Numeric example: `scenario_coalition_reconfiguration.scs` — "Lowering
trust in intelligent_system_ranker: 0.71 → 0.58" (ready before/after pair,
but without a formula for calculating the lowering itself).

### `rules_cooperation.scs` — agent `agent_cooperative_executor`, 3 rules — **✅ fully implemented**

- `rule_dispatch_ready_subtask` — IF assigned∧status=proposed∧∀dependency=completed THEN status=in_progress + send request — **READY** → ✅ implemented, [[AgentDispatchReadySubtask]]
- `rule_register_partial_result` — on receiving r create partial_result+produces+caused_by_subtask+generated_in_context, status=completed — **READY** → ✅ implemented, [[AgentRegisterPartialResult]]
- `rule_propagate_to_dependent` — for each σ' with depends_on(σ',σ) and σ=completed make r an input of σ' — **READY** → ✅ implemented, [[AgentPropagateToDependent]]

All 3 rules are deterministic work with the dependency graph and statuses
(no arithmetic, but with clear IF/THEN) — the form is maximally similar to the
already implemented pattern. **The "cleanest" file of all** — and the only one
currently implemented completely (3 of 3 rules, rather than a mechanical core
for several at once, like [[AgentValidateCardinalityConstraint]]).
Numeric example: `scenario_request_processing.scs` (step 5) — full snapshot
of the `partial_result` structure, reproduced by the integration test
`DispatchesRegistersAndPropagatesAlongSubtaskChain`;
`scenario_knowledge_exchange.scs` — specific payload values
(r_1=42, r_2=37, r_3=51 kW·h) for FIPA-ACL acts.

### `rules_decomposition.scs` — agent `agent_task_decomposer`, 3 rules

- `rule_apply_decomposition_pattern` — argmax sim_T(τ_q,τ_p) over templates — **PARTIAL** (sim_T and template structure not defined)
- `rule_recursive_decomposition` — recursive application of the previous rule — **PARTIAL**
- `rule_check_acyclicity` — after decomposition run acyclic(depends_on) — **READY** (classic graph algorithm) → ✅ implemented, [[AgentCheckAcyclicity]]

Selecting/instantiating the decomposition pattern requires NLP-like semantic
similarity and a template library — not ready as-is.
Acyclicity checking is fully self-contained, and is now implemented as a
DFS over `nrel_subtask_depends_on` restricted to T(q); it does not (and
per the rule's own trigger, cannot) initiate the decomposition-pattern
reconsideration the rule text also asks for, since neither of the other
two rules is implemented.
Numeric example: no explicit numbers, but almost every scenario provides
ready-made subtask DAGs (`nrel_subtask_depends_on`) as test fixtures —
reused directly as the acyclic-case test fixture in
`test_agent_check_acyclicity.cpp`.

### `rules_explanation.scs` — agent `agent_explanation_builder`, 4 rules

- `rule_build_local_explanation` — create `concept_explanation_local` from a provenance template — **PARTIAL** → ✅ structural core implemented, [[AgentBuildExplanation]]
- `rule_build_global_explanation` — aggregate local explanations into a global one — **PARTIAL** → ✅ structural core implemented, [[AgentBuildExplanation]] (aggregation itself still not done)
- `rule_build_contrastive_explanation` — record differences between provenance chains X and Y — **PARTIAL** → ✅ structural core implemented, [[AgentBuildExplanation]] (difference-finding itself still not done)
- `rule_build_counterfactual` — "minimal input change yielding a different result" — **PARTIAL** (open algorithmic problem) → ✅ structural core implemented, [[AgentBuildExplanation]] (minimal-change search itself still not done)

All 4 rules are formally specified (trigger → create entity), and the
common "create entity, reify the construction act with its five roles"
part is now done by [[AgentBuildExplanation]] for all four at once — the
same "one agent, caller supplies the type" pattern as [[AgentComputePiMetric]].
The key type-specific operation ("aggregate", "record differences",
"minimal change") still has no concrete algorithm in the text and remains
the caller's job.
Numeric example: `scenario_explanation_construction.scs` gives an exact scheme
of the reified act (`explanation_act_001`/`explanation_77`), effectively
concretizing the "template" for local/global — reproduced by
`test_agent_build_explanation.cpp`.
`scenario_recommendation.scs` — ready counterfactual example: "if the time
tolerance were increased by 12%, route r_3 would become optimal".

### `rules_integration.scs` — agents `agent_result_integrator` + `agent_conflict_detector`, 5 rules

- `rule_detect_conflict` — `conflict(res_i,res_j) ← inconsistent ∨ incompatible_context ∨ violated_constraint` — **PARTIAL** (3 undefined predicates)
- `rule_aggregate_results` — selecting strategy S taking into account trust(s,τ,h) and C — **PARTIAL**
- `rule_resolve_by_argumentation` — build ⟨Args,Attacks⟩, take grounded extension — **PARTIAL** (grounded extension is a known algorithm, but construction of Args/Attacks is not defined)
- `rule_resolve_by_trust_weighted` — `w_i=trust(s_i,τ,h); r*=arg max_i w_i` — **READY** → ✅ implemented, [[AgentResolveByTrustWeighted]]
- `rule_validate_integration` — `∃e.explained_by(ir,e) ∧ ¬∃c.conflicts_with(ir,c)` — **READY** → ✅ implemented, [[AgentValidateIntegration]]

`rule_resolve_by_trust_weighted` directly reuses the already implemented
[[AgentAggregateTrust|trust(s,τ,h)]], reducing to argmax over ready numbers.
`rule_validate_integration` was a simple check for presence/absence of
links, but only became meaningful once something actually wrote
`nrel_explained_by` — see [[AgentBuildExplanation]]; it checks
`nrel_conflicts_with` in both directions since the relation is symmetric
but not always materialized both ways in KB scenario fixtures.
Numeric example: `scenario_collective_decision.scs` (`[0.91]`),
`scenario_financial_decision_support.scs` (`[0.88]`),
`scenario_recommendation.scs` (`[0.78]`),
`scenario_security_breach_response.scs` (`[0.95]`) — final decision weights
(but not a complete set of competing w_i for argmax).

### `rules_integrity_validation.scs` — agent `agent_integrity_checker`, 7 rules (SHACL shapes) — **✅ fully implemented**

- `rule_validate_authorship_shape` — nrel_formulates⁻¹ minCount1 maxCount1, class concept_user — **READY** → ✅ implemented, [[AgentValidateCardinalityConstraint]]
- `rule_validate_subtask_status_shape` — nrel_subtask_status/priority minCount1 — **READY** → ✅ implemented, [[AgentValidateCardinalityConstraint]]
- `rule_validate_result_provenance_shape` — has_provenance/generated_in_context/obtained_from minCount1 — **READY** → ✅ implemented, [[AgentValidateCardinalityConstraint]]
- `rule_validate_integrated_result_shape` — explained_by minCount1 ∧ ¬conflicts_with — **READY** → ✅ partially implemented (minCount part only), [[AgentValidateCardinalityConstraint]]
- `rule_validate_is_capability_shape` — has_capability_profile minCount1 — **READY** → ✅ partially implemented (minCount part only), [[AgentValidateCardinalityConstraint]]
- `rule_validate_context_completeness` — 5 rrel_*_context minCount1 each — **PARTIAL** (the check itself is ready, but the "fill from history" add-on is not detailed) → ✅ implemented (check), [[AgentValidateCardinalityConstraint]]
- `rule_validate_provenance_chain` — transitive_path derived_from to user_request — **READY** (graph reachability) → ✅ implemented, [[AgentValidateProvenanceChain]] — a fundamentally different task (graph traversal, not edge counting) from the other six

Almost the entire file consists of homogeneous, extremely low-ambiguity
cardinality/existence checks for sc-links — even simpler than the arithmetic
of trust agents, with no undefined subfunctions.
`rule_validate_integrated_result_shape` duplicates `rule_validate_integration`
from `rules_integration.scs` (file overlap) — the latter is now fully
implemented by [[AgentValidateIntegration]], independently of this file's
`minCount`-only partial coverage of the same shape via
[[AgentValidateCardinalityConstraint]].
Numeric example: there is no explicit violation in the scenarios, but
"positive" fixtures (valid request/result with all mandatory links) are
present in almost every scenario.

### `rules_interpretation.scs` — agent `agent_request_interpreter`, 2 rules

- `rule_interpret_request` — `goal_g = ι(q.text, C)` — **NO** (essentially an NLU/LLM task, not a formula)
- `rule_enrich_request` — link q to C_domain via nrel_relevant_to — **NO** (relevance criterion not defined)

The weakest file in terms of "formula-ness" — the key operation
ι(text,C)→goal does not reduce to reading 2–5 numeric sc-links in the spirit
of the already implemented agents. No numeric example.

### `rules_provenance_completion.scs` — agent `agent_provenance_registrar`, 6 rules — **partial (4 of 6 rules)**

- `rule_auto_record_prov_entity` — on appearance of result create prov_entity+wasGeneratedBy+wasAttributedTo — **READY** → ✅ implemented, [[AgentRecordProvEntity]]
- `rule_auto_record_prov_activity` — on process execution create prov_activity(actor,input,timestamp) — **READY** → ✅ implemented, [[AgentRecordProvActivity]]
- `rule_compute_pi_metrics` — `π_proc=|{π:∃prov(π)}|/|Proc|` (similarly π_dec, π_data) — **READY** → ✅ implemented, [[AgentComputePiMetric]]
- `rule_fill_missing_provenance` — reconstruction from log/history/timestamp — **PARTIAL** (heuristic without algorithm)
- `rule_link_explanation_to_provenance` — for each element of use_results/applies_rules create wasDerivedFrom/PROV:used — was **"READY on paper, PARTIAL in practice"** because the PROV-O triad it needs (a `concept_prov_activity` for the explanation act, a `concept_prov_agent` for `agent_explanation_builder`) was not created by any agent. That specific blocker is now removed — [[AgentRecordProvEntity]]/[[AgentRecordProvActivity]] can populate exactly this triad — but the rule itself is still not implemented; closing it would mean wiring [[AgentBuildExplanation]]'s `uses_results`/`applies_rules` role data through to these two agents, a separate integration step not done here
- `rule_verify_transitive_closure` — transitive_closure(derived_from,ir) to request — **READY** → ✅ implemented, [[AgentValidateProvenanceChain]] (duplicates `rule_validate_provenance_chain` in `rules_integrity_validation.scs`, same agent covers both)

**The most "dense" file in terms of readiness** — 5 of 6 rules are
deterministic (counting/ratio of fractions, building a fixed set of
provenance links on trigger, graph reachability); 4 of those 5 are now
implemented ([[AgentComputePiMetric]], [[AgentValidateProvenanceChain]],
[[AgentRecordProvEntity]], [[AgentRecordProvActivity]]).
`rule_compute_pi_metrics` is structurally almost identical to the already
implemented trust aggregation. `rule_auto_record_prov_entity`/
`_activity` were, unlike `rule_choose_modality` elsewhere in this
document, genuinely READY as labeled — every PROV-O class and relation
they need (`concepts/provenance.scs`) already existed, fully aligned
with W3C PROV-O.
Numeric example: `scenario_explanation_construction.scs` — "π_proc=8/8=1.0",
"π_dec=5/5=1.0", "π_data=12/12=1.0" — fully ready calculation with exact
numbers, direct analog of `scenario_trust_assessment.scs`.

### `rules_security_privacy.scs` — no single agent, 7 rules — **partial (2 of 7 rules)**

- `rule_enforce_consent_check` — `∃c.consent_covers(c,pii)∧valid(c)` — **PARTIAL**
- `rule_data_minimization` — payload with only necessary PII, redact the rest — **PARTIAL** (no table of "which PII is needed for which subtask")
- `rule_detect_anomalous_request_rate` — `rate(source,target,Δt) > k_σ·baseline_rate(target)` — **READY** → ✅ implemented, [[AgentDetectAnomalousRequestRate]]
- `rule_audit_log_integrity` — append-only provenance, rewrite attempt → violation — **PARTIAL** (more of a system invariant than a calculation)
- `rule_right_to_explanation` — delivered_to∧request_type∈{...} ⇒ explained_by is mandatory — **READY** → ✅ implemented, [[AgentValidateRightToExplanation]]
- `rule_check_data_residency` — knowledge sources must be in an acceptable jurisdiction r — **PARTIAL** (no data about source jurisdictions in the rest of the KB)
- `rule_apply_access_control_policy` — RBAC/ABAC/capability-based check — **PARTIAL** (3 different unspecified sub-algorithms)

`rule_detect_anomalous_request_rate` is a pure threshold formula,
structurally almost like trust agents; `rule_right_to_explanation` is a simple
gate-check, now implemented as a two-part conjunctive precondition read
directly off existing `nrel_delivered_to`/request-type/`nrel_explained_by`
data. The other 4 require external policies/tables.
Numeric example: `scenario_security_breach_response.scs` — "5400
req/min, exceeds baseline by 27×" — ready input for
`rule_detect_anomalous_request_rate`.

### `rules_user_adaptation.scs` — adaptive interface + `agent_explanation_builder`, 6 rules — **partial (2 of 6 rules)**

- `rule_choose_presentation_template` — cognitive_load(u)≤θ_low∧desktop ⇒ detailed; >θ_high∨mobile ⇒ summary; else ⇒ card — **PARTIAL** (cognitive_load(u) is nowhere computed explicitly)
- `rule_choose_modality` — visual-constraint ⇒ voice; preference_visual ⇒ visual; default ⇒ text — corrected 2026-09-02 from READY to PARTIAL-on-inspection (see note below), then closed by adding the two missing typed flags → ✅ implemented, [[AgentChooseModality]]
- `rule_progressive_disclosure` — depth(provenance)>4 ∨ \|partial_results\|>5 ⇒ executive summary — **READY** (internal graph metrics, no external dependencies) → ✅ implemented, [[AgentEvaluateProgressiveDisclosure]]
- `rule_simplify_terminology_for_end_user` — end_user∧¬domain_expert ⇒ apply glossary — **PARTIAL** (needs a terminology table not defined here)
- `rule_user_feedback_to_user_model` — update competence/preference/history; "recalculate cognitive_load over last 10" — **PARTIAL** (the cognitive_load formula itself is not defined)
- `rule_context_aware_layout` — night-shift/low-light ⇒ dark theme + font ×1.10 — **PARTIAL** ("reduce number of elements" is not quantified, though font ×1.10 is a pure formula)

`rule_progressive_disclosure` is a pure threshold/classification rule
without external dependencies, and is now implemented.
`rule_choose_modality` was originally also assessed READY here
("inputs are Boolean/class flags already existing as sc-facts"), but on
inspection this did not hold up: `concept_user_preference`
(`concepts/users.scs`) was a flat, non-subdivided class — there was no
`concept_preference_visual` or equivalent, and no way to mark a
situational-context node as "visually constrained" anywhere in the
ontology. The one scenario that names a visual preference,
`scenario_intelligent_tutoring.scs`'s `preference_ivan_visual`, carried
that information only in a free-text `nrel_main_idtf` label
(*"Предпочтение: визуальные объяснения с динамическими построениями"*),
not a typed fact. Unlike `rule_choose_presentation_template`'s
`cognitive_load(u)` (a genuinely undefined *formula* — a bigger,
open-ended gap), this rule's gap was two narrow, nameable *flags* the
rule's own prose already specified — closed by adding
`concept_visual_perception_constraint` (`<= nrel_inclusion:
concept_user_situation_context`, `concepts/contexts.scs`) and
`concept_preference_visual_modality` (`<= nrel_inclusion:
concept_user_preference`, `concepts/users.scs`), then implementing
[[AgentChooseModality]] against them. `preference_ivan_visual` now also
carries `<- concept_preference_visual_modality`.
`rule_choose_presentation_template` remains blocked by
`cognitive_load(u)`'s undefined formula — a different, still-open class
of gap.
Numeric example: not found (`cognitive_load`, `θ_low/θ_high` — 0 matches across
all 15 scenarios); `scenario_intelligent_tutoring.scs`'s `preference_ivan_visual`
is now a typed positive fixture for `rule_choose_modality`'s preference
branch.

## Top-5 Most Obvious Candidates

1. ~~`rule_compute_pi_metrics`~~ — ✅ **implemented**, see [[AgentComputePiMetric]].
2. ~~`rule_detect_anomalous_request_rate`~~ — ✅ **implemented**, see [[AgentDetectAnomalousRequestRate]].
3. ~~`rule_resolve_by_trust_weighted`~~ — ✅ **implemented**, see [[AgentResolveByTrustWeighted]].
4. ~~`rule_dispatch_ready_subtask`~~ — ✅ **implemented**, see [[AgentDispatchReadySubtask]].
5. ~~SHACL cardinality checks~~ (`rules_integrity_validation.scs`, 6 homogeneous rules) — ✅ **implemented** by a single general agent, see [[AgentValidateCardinalityConstraint]].

## Not Suitable Now (Requires NLP/Heuristics Without a Formula in the Text)

`rules_interpretation.scs` as a whole and most of `rules_decomposition.scs`
(except `rule_check_acyclicity`) — the key operation like "ι(text,C)→goal"
is essentially an LLM task, not a computable formula fixed in the KB text.
`rules_explanation.scs` no longer belongs here in full: its structural
core is implemented ([[AgentBuildExplanation]]); only the per-type
semantic content (aggregation/difference-finding/minimal-change search)
remains an LLM-shaped task.
