---
type: moc
tags: [moc, trust-agents, provenance-agents]
---

# Ontological descriptions of ostis_ps_user_ecosystem agents

The `TrustModule` module ([[ostis_ps_user_ecosystem]]) contains twelve sc-agents:
five implement the rules for updating the trust vector π_tr, one —
the computable part of provenance completeness metrics, one — detection
of anomalous request frequency, one — resolution of result conflicts by
weighted trust, three — the full life cycle of cooperative
subtask execution (dispatch → result registration →
propagation to dependent subtasks), one — the mechanical core
of SHACL cardinality checks. Code: `ostis_ps_user_ecosystem/src/agent/`.
Rule ontology: `ostis_kb_user_ecosystem/rules/`.

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

## How to read these notes

Each agent note follows a fixed template: Identification →
Purpose → Competence → Input → Output → Relations →
Behavior → Security → Metrics → Tests. The «Security» section and the
«Limitations» part of «Competence» are derived from analyzing the real code
(`m_context.IsElement(...)` — the only argument check), not from
guesswork — they are deliberate, verified limitations of the current
implementation, not generic statements.

## Cross-cutting dependencies

All twelve agents use:
- `common/float_link_io.hpp` — shared `WriteFloatLinkContent` (and
  `ReadFloatLinkContent` — all except [[AgentComputePiMetric]], which
  only needs to know that the link exists, and
  [[AgentRegisterPartialResult]], which has no numeric indicator).
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
- `keynodes/trust_keynodes.hpp` (`TrustKeynodes`) / `keynodes/provenance_keynodes.hpp`
  (`ProvenanceKeynodes`) / `keynodes/security_keynodes.hpp`
  (`SecurityKeynodes`) / `keynodes/integration_keynodes.hpp`
  (`IntegrationKeynodes`) / `keynodes/task_keynodes.hpp`
  (`TaskKeynodes`) / `keynodes/integrity_keynodes.hpp`
  (`IntegrityKeynodes`) — key sc-elements and action classes.
- Base class `ScActionInitiatedAgent` (sc-machine) — reacts to the
  event `ScEventAfterGenerateOutgoingArc` with subscription element
  `action_initiated`.
- The same C++ module `TrustModule` — sc-machine allows only
  one module (`SC_MODULE_REGISTER`) per shared library, so
  [[AgentComputePiMetric]], [[AgentDetectAnomalousRequestRate]],
  [[AgentResolveByTrustWeighted]], [[AgentDispatchReadySubtask]],
  [[AgentValidateCardinalityConstraint]], [[AgentRegisterPartialResult]]
  and [[AgentPropagateToDependent]] are registered there too, despite
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

## Running agents standalone

`ostis_ps_user_ecosystem/scripts/run_agent.py` -- a Python script on top of
py-sc-client (venv sc-web) for launching any of the 12 agents against a live
sc-server without C++ tests and without a browser JS console; by default
it runs demo scenarios for all 12 agents in a row using illustrative
numbers from tests/test_agent_*.cpp. Launch details are in the docstring
of the file itself.

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

Both tests show that each agent's output is correctly read by the next one
through data — without a separate orchestrator agent, which the project
does not yet have (see above about [[AgentAggregateTrust]]).

## What's next

[[Chapter 2 Alignment]] — the implementation was frozen 2026-08-22, chapter 2
was revised 2026-08-24, and two of those revisions reach the code: the default
trust weights become `λ_k = 1/4`, and the four smoothing coefficients α/β/γ/δ
become a single `η` that also has to be calibratable at run time. Five agents
affected, seven verified unaffected. Nothing applied yet.

[[Implementation Backlog]] — an overview of the remaining six unimplemented
agents from the catalog `concepts/sc_agents.scs`. `agent_trust_evaluator`
and `agent_cooperative_executor` are fully implemented;
`agent_provenance_registrar`, the F_rule_sec security subsystem,
`agent_result_integrator`/`agent_conflict_detector` and
`agent_integrity_checker` are partially implemented, with a readiness
assessment for each rule.
