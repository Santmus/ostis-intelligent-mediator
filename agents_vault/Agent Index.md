---
type: moc
tags: [moc, trust-agents, provenance-agents]
---

# Ontological descriptions of ostis_ps_user_ecosystem agents

The `TrustModule` module ([[ostis_ps_user_ecosystem]]) contains
twenty-seven sc-agents: five implement the rules for updating the trust
vector π_tr, one — the computable part of provenance completeness
metrics, one — detection of anomalous request frequency, one —
resolution of result conflicts by weighted trust, three — the full life
cycle of cooperative subtask execution (dispatch → result registration →
propagation to dependent subtasks), one — the mechanical core
of SHACL cardinality checks, one — structural reification of explanation
construction acts, one — validation of an integrated result (explanation +
conflicts) before publication, one — transitive-closure validation of a
result's provenance chain back to the originating request, two —
coalition reconfiguration on system failure and resource exhaustion, one —
acyclicity checking of the subtask dependency graph after decomposition,
one — gating delivery on the right-to-explanation property for
decision/recommendation requests, one — deciding whether a provenance
chain/result set is complex enough to require progressive disclosure,
one — choosing an interaction modality from situational and preference
flags, two — populating the PROV-O provenance triad (entity/activity),
one — deciding between decomposition and an integrity violation when no
system is an admissible executor, one — detecting a night-shift/low-light
situational trigger for context-aware layout, one — wiring an
explanation-construction act's used-results/applied-rules role data into
the PROV-O relations the entity/activity/agent triad already supports,
and one — mechanical recording of a request's lifecycle state
transition (Phase 1 of the orchestrator gap, AGENTS_AUDIT.md §7.1).
Code: `ostis_ps_user_ecosystem/src/agent/`. Rule ontology:
`ostis_kb_user_ecosystem/rules/`.

## Agent map

| Agent | Action | Updates | Type |
|---|---|---|---|
| [[AgentUpdateTrustReliability]] | `action_update_trust_reliability` | t_rel | smoothing (EMA) |
| [[AgentUpdateTrustAccuracy]] | `action_update_trust_accuracy` | t_acc | smoothing (EMA) |
| [[AgentUpdateTrustSecurity]] | `action_update_trust_security` | t_sec | smoothing (EMA) |
| [[AgentUpdateTrustTransparency]] | `action_update_trust_transparency` | t_tr | smoothing (EMA) |
| [[AgentAggregateTrust]] | `action_aggregate_trust` | trust(s,τ,h) | weighted aggregation |
| [[AgentComputePiMetric]] | `action_compute_pi_metric` | π_proc / π_dec / π_data | provenance-covered fraction |
| [[AgentDetectAnomalousRequestRate]] | `action_detect_anomalous_request_rate` | anomaly indicator + incident | threshold check |
| [[AgentResolveByTrustWeighted]] | `action_resolve_by_trust_weighted` | integrated_result | argmax by trust |
| [[AgentDispatchReadySubtask]] | `action_dispatch_ready_subtask` | subtask_status + act_request | state transition |
| [[AgentValidateCardinalityConstraint]] | `action_validate_cardinality_constraint` | indicator + integrity_violation | arc count vs [min,max] |
| [[AgentRegisterPartialResult]] | `action_register_partial_result` | partial_result + provenance triple + subtask_status | unconditional commit |
| [[AgentPropagateToDependent]] | `action_propagate_to_dependent` | act_inform for each dependent σ' | reverse dependency-graph traversal |
| [[AgentBuildExplanation]] | `action_build_explanation` | explanation + explanation_construction_act | structural reification |
| [[AgentValidateIntegration]] | `action_validate_integration` | indicator | explanation ∧ ¬conflict gate |
| [[AgentValidateProvenanceChain]] | `action_validate_provenance_chain` | indicator + integrity_violation | BFS reachability to user_request |
| [[AgentExcludeFromCoalition]] | `action_exclude_from_coalition` | `nrel_member_of` (erased) + indicator | conditional state transition |
| [[AgentReassignOnResourceExhaustion]] | `action_reassign_on_resource_exhaustion` | `nrel_subtask_status` (bulk) + count | reverse-index bulk update |
| [[AgentCheckAcyclicity]] | `action_check_acyclicity` | indicator + integrity_violation | DFS cycle detection |
| [[AgentValidateRightToExplanation]] | `action_validate_right_to_explanation` | indicator + integrity_violation | conditional gate check |
| [[AgentEvaluateProgressiveDisclosure]] | `action_evaluate_progressive_disclosure` | indicator (decision, not validity) | two-threshold decision over DFS depth + count |
| [[AgentChooseModality]] | `action_choose_modality` | chosen modality class (not numeric) | three-way branch over two new ontology flags |
| [[AgentRecordProvEntity]] | `action_record_prov_entity` | `concept_prov_entity` + `nrel_has_provenance` | PROV-O construction |
| [[AgentRecordProvActivity]] | `action_record_prov_activity` | `concept_prov_activity` + roles | PROV-O construction |
| [[AgentAllocateSubtasks]] | `action_allocate_subtasks` | `nrel_assigned_to` + reified `concept_process_allocation` acts | 3-of-4 admissibility gate + top-k selection (step 2.3) |
| [[AgentHandleNoAdmissibleAssignment]] | `action_handle_no_admissible_assignment` | indicator + integrity_violation | binary branch on subtask class |
| [[AgentEvaluateContextAwareLayout]] | `action_evaluate_context_aware_layout` | indicator (decision) + strategy marker | two-flag OR gate over two new ontology flags |
| [[AgentLinkExplanationToProvenance]] | `action_link_explanation_to_provenance` | `nrel_prov_was_derived_from`/`nrel_prov_used`/`nrel_prov_was_attributed_to` | role-to-relation wiring |
| [[AgentRecordStateTransition]] | `action_record_state_transition` | `nrel_has_state` + `concept_state_transition` + `AuditRecord`/`ProvenanceRecord` | admissibility + resource gate, mechanical bundle construction |

## How to read these notes

Each agent note follows a fixed template: Identification →
Purpose → Competence → Input → Output → Relations →
Behavior → Security → Metrics → Tests. The «Security» section and the
«Limitations» part of «Competence» are derived from analyzing the real code
(`m_context.IsElement(...)` — the only argument check), not from
guesswork — they are deliberate, verified limitations of the current
implementation, not generic statements.

## Cross-cutting dependencies

All twenty-seven agents use:
- `common/float_link_io.hpp` — shared `WriteFloatLinkContent` (and
  `ReadFloatLinkContent` — all except [[AgentComputePiMetric]], which
  only needs to know that the link exists,
  [[AgentRegisterPartialResult]], which has no numeric indicator, and
  [[AgentBuildExplanation]], which writes no numeric indicator at all).
- `common/relation_io.hpp` — shared `TargetOf`/`LinkByRelation` for
  finding/creating non-role binary relations (`nrel_*`); introduced when
  implementing [[AgentResolveByTrustWeighted]], extracted into a common
  header when reused in [[AgentDispatchReadySubtask]]. When implementing
  [[AgentRegisterPartialResult]] and
  [[AgentPropagateToDependent]], `SetFunctionalRelationTarget`
  (a generalization of the local `SetFunctionalStatus` from
  [[AgentDispatchReadySubtask]]) and `LinkByRole` (a generalization of the
  helper of the same name from the same place) were also moved there — the
  same reuse pattern when a second/third consumer appeared.
  [[AgentBuildExplanation]] is the first agent to reuse `LinkByRole` for a
  five-role reification without introducing any new helper.
  [[AgentLinkExplanationToProvenance]] adds `ForEachRoleTarget`, the
  read-symmetric counterpart to `LinkByRole` — no prior agent needed to
  iterate *every* target of a role from one source rather than write one
  (`LinkByRole`) or iterate a plain group's members
  (`LinkGroupMembersByRole`, a local helper in
  `agent_build_explanation.cpp`, not shared).
  [[AgentRecordStateTransition]] reuses `TargetOf`/`LinkByRelation`/
  `LinkByRole`/`SetFunctionalRelationTarget` as-is — no new shared
  helper, the first agent since [[AgentBuildExplanation]] to need this
  many of them at once without adding anything.
- `keynodes/trust_keynodes.hpp` (`TrustKeynodes`) / `keynodes/provenance_keynodes.hpp`
  (`ProvenanceKeynodes`) / `keynodes/security_keynodes.hpp`
  (`SecurityKeynodes`) / `keynodes/integration_keynodes.hpp`
  (`IntegrationKeynodes`) / `keynodes/task_keynodes.hpp`
  (`TaskKeynodes`) / `keynodes/integrity_keynodes.hpp`
  (`IntegrityKeynodes`) / `keynodes/explanation_keynodes.hpp`
  (`ExplanationKeynodes`) / `keynodes/coalition_keynodes.hpp`
  (`CoalitionKeynodes`) / `keynodes/adaptive_interface_keynodes.hpp`
  (`AdaptiveInterfaceKeynodes`) / `keynodes/allocation_keynodes.hpp`
  (`AllocationKeynodes`) / `keynodes/lifecycle_keynodes.hpp`
  (`LifecycleKeynodes`) — key sc-elements and action classes.
- Base class `ScActionInitiatedAgent` (sc-machine) — reacts to the
  event `ScEventAfterGenerateOutgoingArc` with subscription element
  `action_initiated`.
- The same C++ module `TrustModule` — sc-machine allows only
  one module (`SC_MODULE_REGISTER`) per shared library, so
  [[AgentComputePiMetric]], [[AgentDetectAnomalousRequestRate]],
  [[AgentResolveByTrustWeighted]], [[AgentDispatchReadySubtask]],
  [[AgentValidateCardinalityConstraint]], [[AgentRegisterPartialResult]],
  [[AgentPropagateToDependent]], [[AgentBuildExplanation]],
  [[AgentValidateIntegration]], [[AgentValidateProvenanceChain]],
  [[AgentExcludeFromCoalition]], [[AgentReassignOnResourceExhaustion]],
  [[AgentCheckAcyclicity]], [[AgentValidateRightToExplanation]],
  [[AgentEvaluateProgressiveDisclosure]], [[AgentChooseModality]],
  [[AgentRecordProvEntity]], [[AgentRecordProvActivity]],
  [[AgentHandleNoAdmissibleAssignment]],
  [[AgentEvaluateContextAwareLayout]],
  [[AgentLinkExplanationToProvenance]] and
  [[AgentRecordStateTransition]] are registered there too, despite
  the name.

[[AgentAggregateTrust]] depends on the results of the other three EMA
agents (not [[AgentComputePiMetric]]/[[AgentDetectAnomalousRequestRate]]) —
it reads their `value_link`, but does not initiate them itself (call order
is the external orchestrator's responsibility). [[AgentResolveByTrustWeighted]]
in turn reads the same `trust_level` that is produced by
[[AgentAggregateTrust]] — the only data link between the
agent «families» in the project.
[[AgentDispatchReadySubtask]], [[AgentRegisterPartialResult]] and
[[AgentPropagateToDependent]] are data-independent from the other seven agents —
the only trio operating in the «subtask»
domain (`concept_subtask`), but linked to each other by the state of the same
subtask σ (`nrel_subtask_status`): Dispatch moves it
`proposed → in_progress`, Register — `in_progress → completed` (plus
a provenance triple for the result), Propagate reads exactly `completed`
as the precondition for sending. The call order among them is the responsibility
of external code/orchestrator, as with the EMA agents; the end-to-end test
`test_integration_request_pipeline.cpp` reproduces this entire chain
(see «End-to-end test» below).
[[AgentValidateCardinalityConstraint]] is also data-independent —
the only agent operating in the «KB integrity» domain
(`concept_integrity_constraint`/`concept_integrity_violation`), and
the only one that implements not a single rule but a mechanical core
shared by six SHACL shapes at once.
[[AgentBuildExplanation]] is data-independent from the twelve trust/
subtask/integrity agents — the only agent in the «explanation» domain
(`concept_explanation`/`concept_explanation_construction_act`) — and,
like [[AgentValidateCardinalityConstraint]], implements the shared
structural core of four rules (`rule_build_local_explanation`/`_global`/
`_contrastive`/`_counterfactual`) rather than one. It is not yet called
by [[AgentResolveByTrustWeighted]] or any other agent — see its own
Limitations. It does, however, feed [[AgentValidateIntegration]]: the
latter's `explained_by(ir,e)` check is only ever satisfied by an arc that
[[AgentBuildExplanation]] (or [[AgentResolveByTrustWeighted]]'s own inline
placeholder) actually created — the only data link crossing from the
«explanation» domain into «integration». [[AgentValidateIntegration]] also
reads `nrel_conflicts_with`, which no agent in the project currently
writes (only present as fixture data in KB scenarios) — so in practice its
"has_conflict" branch is exercised by tests, not by any other agent's
output.
[[AgentValidateProvenanceChain]] is fully data-independent — it reads
`nrel_derived_from`, which (like `nrel_conflicts_with`) no agent in the
project writes; it is the first agent to perform an unbounded graph
traversal rather than a single-hop lookup, and the only one whose tests
include a cyclic-graph fixture to prove termination.
[[AgentExcludeFromCoalition]] and [[AgentReassignOnResourceExhaustion]]
are the first agents outside `agent_cooperative_executor`'s trio to
mutate `nrel_subtask_status` — but they are triggered by *failure*
(`concept_subtask_status_failed` → excluded from coalition,
resource exhaustion → `reassigned`), the opposite end of the status
lifecycle from [[AgentDispatchReadySubtask]]/[[AgentRegisterPartialResult]]'s
`proposed → in_progress → completed` happy path. Both are otherwise
data-independent from the rest of the module — the only two operating on
`nrel_member_of`/coalition membership.
[[AgentCheckAcyclicity]] is the first agent implemented for
`agent_task_decomposer`/`rules_decomposition.scs` — data-independent from
the rest of the module, it reads `nrel_decomposed_into` (newly added to
`TaskKeynodes`) and the same `nrel_subtask_depends_on` relation
[[AgentDispatchReadySubtask]]/[[AgentPropagateToDependent]] already use,
but for cycle detection rather than readiness/propagation lookups. Like
[[AgentValidateProvenanceChain]] it performs a multi-hop graph traversal
rather than a single-hop lookup, but distinguishes `onStack` from
`visited` (DFS-style) instead of a single visited set (BFS-style),
because reachability alone is not enough to tell a cycle apart from a
converging (diamond-shaped) DAG.
[[AgentValidateRightToExplanation]] is the second agent for
`rules_security_privacy.scs`, alongside [[AgentDetectAnomalousRequestRate]].
It is the first agent whose violation node is classified by two classes
at once (`concept_integrity_violation` and the already-existing
`concept_property_right_to_explanation`) rather than by one newly
declared subclass — no new ontology class was needed, since
`concepts/security_privacy.scs` already had a specific-enough class for
this property. It reads `nrel_explained_by`, the same relation
[[AgentValidateIntegration]] checks and [[AgentBuildExplanation]]
writes, but is data-independent from both — no agent in the module calls
another agent directly.
[[AgentEvaluateProgressiveDisclosure]] is the first agent for
`rules_user_adaptation.scs` — a genuinely new `keynodes/adaptive_interface_keynodes.hpp`
file was created for it, the smallest keynodes file in the project (one
action class), matching how `coalition_keynodes.hpp` started before
[[AgentReassignOnResourceExhaustion]] joined [[AgentExcludeFromCoalition]]
in it. It is also the first agent whose indicator is a decision rather
than a validity judgment (`1.0` = "the rule fired", not "no violation") —
deliberately flagged in its own doc so this doesn't get read as the
usual `AgentValidate*` polarity. Reuses `nrel_derived_from`
([[AgentValidateProvenanceChain]]'s relation) for a longest-path depth
computation rather than reachability, and `elements_group`-style
membership counting ([[AgentComputePiMetric]]'s convention) for the
partial-result threshold — the first agent to combine both
already-established traversal idioms in one check.
[[AgentChooseModality]] closes `rule_choose_modality`, the other rule of
`rules_user_adaptation.scs`. It was originally flagged READY in
[[Implementation Backlog]] but, on inspection, was not: neither of its
two branch conditions ("ограничение визуального восприятия" on
`rrel_user_situation_context`, "preference_visual" on a
`concept_user_preference` instance) had any typed sc-representation
anywhere in the ontology — `concept_user_preference` was a flat,
non-subdivided class, and the one scenario fixture that names a visual
preference (`scenario_intelligent_tutoring.scs`'s `preference_ivan_visual`)
carried the information only in a free-text `nrel_main_idtf` label.
Finishing this rule therefore required two small, targeted ontology
additions first — `concept_visual_perception_constraint`
(`<= nrel_inclusion: concept_user_situation_context`,
`concepts/contexts.scs`) and `concept_preference_visual_modality`
(`<= nrel_inclusion: concept_user_preference`, `concepts/users.scs`) —
narrow subclasses naming exactly what the rule's own prose already
described, not new semantics. The existing `preference_ivan_visual`
fixture was updated to carry the new typed classification alongside its
original free-text label. [[AgentChooseModality]] is also the second
agent (after [[AgentBuildExplanation]]) whose result is a chosen
sc-element rather than a numeric indicator — `FormResult` returns the
selected modality class directly.
[[AgentRecordProvEntity]] and [[AgentRecordProvActivity]] populate the
PROV-O triad (`concept_prov_entity`/`_activity`/`_agent`,
`concepts/provenance.scs`) that had been fully typed since before this
implementation pass but never written to by any agent — closing
`rule_auto_record_prov_entity`/`rule_auto_record_prov_activity`, both
labeled READY in [[Implementation Backlog]] and, unlike
`rule_choose_modality`, genuinely were: every class and relation they
need already existed. Together they are what
`rule_link_explanation_to_provenance` was blocked on (see
[[Implementation Backlog]]'s own note on that rule — it needed a typed
`concept_prov_activity` for the explanation act, which nothing created
until now). [[AgentRecordProvEntity]] also surfaces a pre-existing
tension worth noting: [[AgentRegisterPartialResult]] already creates a
generic (non-PROV-O-typed) `concept_provenance_record` and sets it as a
result's `nrel_has_provenance` target; since that relation is
functional, calling [[AgentRecordProvEntity]] afterward on the same
result replaces it with the PROV-O-specific entity. Neither the model
text nor this implementation pass says which mechanism is canonical —
documented as an open question, not resolved.
[[AgentHandleNoAdmissibleAssignment]] is the first (and, per
[[Implementation Backlog]], last remaining READY-and-unimplemented)
agent for `agent_task_allocator`/`rules_allocation.scs` — a new
`keynodes/allocation_keynodes.hpp` file, minimal like
`coalition_keynodes.hpp`/`adaptive_interface_keynodes.hpp` were at
their first agent. It trusts the caller for the rule's premise (no
system is an admissible executor for the subtask — `match()` is itself
only partially defined, see [[Implementation Backlog]]'s own note on
`rules_allocation.scs`) the same way
[[AgentExcludeFromCoalition]]/[[AgentReassignOnResourceExhaustion]]
trust `failed`/`resource_exhausted`, and resolves the rule's two-path
consequent via `affirmation_subtask_executability`
(`concepts/tasks.scs`: every subtask is executable or decomposable) —
checking `concept_subtask_composite` rather than re-deriving anything.
Data-independent from the rest of the module.
[[AgentEvaluateContextAwareLayout]] closes `rule_context_aware_layout`,
the third agent for `rules_user_adaptation.scs`. Its input gap matched
`rule_choose_modality`'s shape closely enough to reuse the same method
— two narrow flags on `concept_user_situation_context`
(`concept_night_shift_context`, `concept_low_light_context`) — but its
consequent does not: "reduce the number of simultaneously shown
elements" is genuinely unquantified in the text, the same open-ended
gap as `cognitive_load(u)`, so only the *detection* half is closed. The
triggered case is materialized by attaching the pre-existing
`concept_strategy_context_aware_layout` (`concepts/adaptive_interface.scs`)
as a single marker standing in for the whole consequent package — that
class had named the strategy since before this implementation pass but
had no producer until now. Data-independent from the rest of the
module, including its two `rules_user_adaptation.scs` siblings.
[[AgentLinkExplanationToProvenance]] closes `rule_link_explanation_to_provenance`
— unlike every other agent added in this implementation pass, it needed
*zero* new ontology: `nrel_prov_was_derived_from`/`nrel_prov_used`/
`nrel_prov_was_attributed_to` and `rrel_uses_results`/`rrel_applies_rules`
all already existed, and the rule's only real blocker (nothing had ever
created the PROV-O entity/activity/agent nodes to wire together) was
removed as a side effect of [[AgentRecordProvEntity]]/
[[AgentRecordProvActivity]] the day before (2026-09-02). Reads
`rrel_uses_results`/`rrel_applies_rules` directly off the
`concept_explanation_construction_act` [[AgentBuildExplanation]]
produces and writes the three named relations against caller-supplied
PROV-O nodes, without attempting to type the used results/applied rules
themselves as `concept_prov_entity` — the rule names them directly as
the relation's target, and [[AgentBuildExplanation]]'s own Limitations
had already flagged doing that typing as "introducing unverified
ontology." Downstream of [[AgentBuildExplanation]] and, for its
arguments to be meaningfully typed, typically of
[[AgentRecordProvEntity]]/[[AgentRecordProvActivity]] as well — though
it calls neither directly. `rules_provenance_completion.scs` is now
5 of 6 rules implemented — only `rule_fill_missing_provenance` (a
heuristic reconstruction with no algorithm in the text) remains.
[[AgentRecordStateTransition]] is Phase 1 of AGENTS_AUDIT.md §7.1, the
lifecycle-orchestrator gap — chosen over the other two equally-sized
missing subsystems (an external-system bridge, an ontology-translation
service) because its ontology already existed in full and was read by
none of the other twenty-six agents. It is the first agent in the
module to implement a `constraint_*`/`affirmation_*` text
(`axioms/axioms_constraints.scs`, `concepts/mediation_lifecycle.scs`)
rather than a `rule_*` — a new `rules/rules_mediation_orchestration.scs`
declares `action_record_state_transition` for it, since no existing
rules file was the right home. Deliberately mechanical, not a deciding
orchestrator: the caller picks `to_state_class`, matching the
project-wide rule that no agent calls another agent's action (see
`test_integration_request_pipeline.cpp`'s third `TEST_F`, where this
agent brackets a call to [[AgentValidateIntegration]] without either
calling the other). Scoped to the slice of the pipeline that already
has real agents on both sides — `x_exec`/`x_aggregate`/`x_verify` plus
the `x_exec → x_assign` reassignment edge — so its correctness is
demonstrable against agents that actually produce and consume those
states today, rather than against states nothing populates yet. Reuses
`TargetOf`/`LinkByRelation`/`LinkByRole`/`SetFunctionalRelationTarget`
unchanged; the new `keynodes/lifecycle_keynodes.hpp` introduces no KB
element that didn't already exist. The other nine lifecycle states, the
replanning cycle (`d_max`), and — most importantly — anything that
decides *when* to call this agent remain open (Phase 2, not started).

## Running agents standalone

`ostis_ps_user_ecosystem/scripts/run_agent.py` -- a Python script on top of
py-sc-client (venv sc-web) for launching any of the 27 agents against a live
sc-server without C++ tests and without a browser JS console; by default
it runs demo scenarios for all 27 agents in a row using illustrative
numbers from tests/test_agent_*.cpp. Launch details are in the docstring
of the file itself. Extended to 24 (from an original 12) during the
ontology/article/agent consistency check on 2026-09-02, which also caught
`demo_aggregate_trust` computing its expected value against the
pre-2026-08-30 unequal trust weights instead of the current λ_k = 1/4 --
fixed alongside that extension. Extended again to 26 on 2026-09-03 for
[[AgentEvaluateContextAwareLayout]]/[[AgentLinkExplanationToProvenance]],
then to 27 the same day for [[AgentRecordStateTransition]] — all three
verified only via `py_compile`/AST introspection (27 `DEMOS` entries, 24
`demo_` functions since `demo_trust_ema` is reused 4×), not against a
live sc-server: `demo_record_state_transition` specifically was written
against a `sc-server` that had already been running 4+ hours on an older
KB/agent build, so restarting it with `--rebuild` to actually exercise
the new demo was deliberately left to whoever next has a reason to
restart that process, rather than done unprompted mid-session (see
`RUNNING_AGENTS.md`'s "Известные ограничения").

> [!warning] Discovered sc-machine bug: do not erase the action node after InitiateAndWait
> Erasing the sc-node of a finished action (`action_addr`, after it
> already has an arc `action_finished_successfully`/...) via
> `erase_elements` reliably crashes the entire sc-server: an unhandled
> `utils::ExceptionInvalidParams` in C++ (`libc++abi: terminating due to
> uncaught exception`), the sc-server process terminates. Reproduced
> repeatedly while preparing `run_agent.py`, with the client's standard
> request sequence, without a single line of TrustModule agent code —
> it looks like a bug in sc-machine's internal bookkeeping, not in this
> project. `run_agent.py` now deliberately does NOT erase `action_addr`
> (see the `run_action` docstring) -- use the same technique in any
> other script/client that invokes these agents from outside the C++ tests.

## End-to-end test

`ostis_ps_user_ecosystem/tests/test_integration_request_pipeline.cpp` — not
an agent, but two tests that manually wire ready-made agents in sequence.

**`ValidatesUpdatesAggregatesAndResolvesConflict`** — four agents
within the fragment `scenario_request_processing.scs`:
[[AgentValidateCardinalityConstraint]] (exactly one request author) →
[[AgentUpdateTrustReliability]] (positive feedback raises
t_rel of the system that processed the request from zero) → [[AgentAggregateTrust]]
(aggregates into trust(s,τ,h) = 0.04) → [[AgentResolveByTrustWeighted]]
(this system wins the conflict against a competitor with trust 0.01).

**`DispatchesRegistersAndPropagatesAlongSubtaskChain`** — the full life
cycle of `agent_cooperative_executor` along the subtask chain σ_dep
(already completed) → σ → σ': [[AgentDispatchReadySubtask]] (σ is assigned,
`proposed`, its only dependency σ_dep is already `completed` — ready to run) →
[[AgentRegisterPartialResult]] (the system sends result r, provenance
is registered, σ → `completed`) →
[[AgentPropagateToDependent]] (σ', which depends on σ, receives r as input
via an inform act).

**`RecordsStateTransitionsAroundIntegrationValidation`** — extends the
same subtask-chain fragment as the previous test
([[AgentDispatchReadySubtask]] → [[AgentRegisterPartialResult]]) with
lifecycle bookkeeping around an integration check:
[[AgentRecordStateTransition]] (`x_exec → x_aggregate`) →
[[AgentValidateIntegration]] (the integrated result is explained and
conflict-free) → [[AgentRecordStateTransition]] (`x_aggregate →
x_verify`). Walking `rrel_to_state`/`rrel_from_state` back across both
transition nodes reconstructs `x_exec → x_aggregate → x_verify` —
`affirmation_transition_traceability` (`concepts/mediation_lifecycle.scs`)
demonstrated directly, not just asserted per-transition.

All three tests show that each agent's output is correctly read by the
next one through data — without a separate orchestrator agent, which the
project does not yet have (see above about [[AgentAggregateTrust]]);
[[AgentRecordStateTransition]] itself is not that orchestrator either,
by design (see its own note above).

## Ontological agent declarations

`ostis_kb_user_ecosystem/concepts/sc_agent_realizations.scs` — added
2026-09-03, after a comparative study of `ostis-web-platform`/`ims.ostis.kb`
found that none of the 26 agents were declared the way OSTIS itself
expects: a node `<- abstract_sc_agent` carrying
`nrel_primary_initiation_condition`, `nrel_initiation_condition_and_result`
and `nrel_sc_agent_key_sc_elements`, with a realization
`<- platform_dependent_abstract_sc_agent` pointing at the actual source.
Verified against 90+ real examples in `ims.ostis.kb`, most directly
`.../lib_c_agents/{erase_elements,search_of_all_input_arcs}/*.scs` —
structurally identical C++-realization agents. Before this file, every
agent worked only through the C++ `ScActionInitiatedAgent` subscription,
invisible to anything that discovers agents through the ontology itself
(a platform tool, or a scp-agent searching for `abstract_sc_agent`).

Each of the (now 27) agents gets a `sc_agent_of_<name>` node with the
full required relation set; argument positions in the condition/result
templates use `rrel_1..rrel_N` (N taken from that agent's own
`GetArguments<N>()`), matching how `ScAction::SetArgument` actually
resolves argument index to `ScKeynodes::GetRrelIndex(idx)`. This is pure
formalization — no agent's behavior changed, no new domain semantics —
and it complements rather than replaces `concepts/sc_agents.scs` (that
file models the ~10-12 agent *families* at the lifecycle-process level;
this one registers all concrete *realizations* at the platform level).
Extended same-day (2026-09-03) for `sc_agent_of_record_state_transition`
when [[AgentRecordStateTransition]] was added a few hours later — the
27th realization, alphabetically between
`sc_agent_of_record_prov_entity` and `sc_agent_of_register_partial_result`.

## What's next

Chapter 2 alignment — applied 2026-08-30. The implementation was frozen 2026-08-22, chapter 2
was revised 2026-08-24, and two of those revisions reached the code: the default
trust weights became `λ_k = 1/4`, and the four smoothing coefficients α/β/γ/δ
became a single `η`, calibratable at run time as an optional action argument.
Five agents affected, seven verified unaffected.

[[Implementation Backlog]] — an overview of the remaining agents from the
catalog `concepts/sc_agents.scs`. `agent_trust_evaluator`,
`agent_cooperative_executor` and `agent_integrity_checker` are fully
implemented; `agent_provenance_registrar`, the F_rule_sec security
subsystem, `agent_result_integrator`/`agent_conflict_detector`,
`agent_explanation_builder`, `agent_coalition_reconfigurator`,
`agent_task_decomposer`, `agent_task_allocator` and the F_rule_adapt
adaptive-interface rules (no single owner in the catalog) are partially
implemented, with a readiness assessment for each rule.

Lifecycle orchestrator, Phase 1 — applied 2026-09-03
([[AgentRecordStateTransition]], AGENTS_AUDIT.md §5.20/§7.1). A plan
session comparing three equally-sized missing subsystems (orchestrator,
external-system bridge, ontology-translation service) picked the
orchestrator first since its ontology already existed and nothing read
it. Phase 2 — started 2026-09-13 with [[AgentAllocateSubtasks]], the §7.5
agent the audit singled out as unblocking both §7.3 and §7.4; it was
designed through a five-seat commission rather than unilaterally
(protocol: [[AgentAllocateSubtasks Commission]]). The remaining
fully-missing §7.5 agents (`AgentInterpretRequest`, `AgentBuildPlan`,
`AgentExecutionMonitor`, and others) plus the `x_replan`/`d_max` cycle
are still open; each is a task of comparable size. The ratified next
item is `AgentComputeSimType` — the Wu–Palmer `sim_T` measure
(`eq:sim_type`), the only term of `match` formalized closely enough in
chapter 2 to implement without inventing a formula, and absent from the
KB entirely. Phase 3 (external-system bridge) and Phase 4
(ontology-translation service) remain unstarted and undesigned beyond
their KB class declarations.
