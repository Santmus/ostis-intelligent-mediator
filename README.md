# ostis-intelligent-mediator

Users today interact not with a single intelligent system but with a
community of them — and are forced to decide whom to address, how to
combine heterogeneous answers, and whom to trust when they contradict.
This project lifts that coordination into a separate layer: an
intelligent mediator that accepts a goal in the domain language rather
than a command in the interface language. Unlike classical mediators
(e.g., Wiederhold et al.), which mediate data and their representations,
here the user's intention and the competencies of the executors are
mediated, while the set of participants remains open.

This repository is an operationalization of formal rules on the
[OSTIS](https://github.com/ostis-ai) platform: a mediator ontology in SCs
and a C++ sc-agent module on top of
[sc-machine](https://github.com/ostis-ai/sc-machine).

## Repository structure

```
.
├── ostis_kb_user_ecosystem/   # ontology (SCs): concepts, relations, rules, axioms, scenarios
├── ostis_ps_user_ecosystem/   # program system (C++): TrustModule sc-agents + tests
└── agents_vault/              # Obsidian vault: ontological documentation for each agent
```

### `ostis_kb_user_ecosystem/` — knowledge base

SCs sources intended to be built with `sc-builder` as part of
`ostis-web-platform`:

- `concepts/` — basic concepts (including `concept_sc_agent` and the
  catalog of 12 agents);
- `relations/`, `definitions/`, `axioms/` — ontology relations and
  formal apparatus;
- `rules/` — formal rules for each agent (e.g.,
  `rules_trust_update.scs`, `rules_cooperation.scs`,
  `rules_integrity_validation.scs`);
- `scenarios/`, `examples/` — illustrative scenarios used as fixtures in
  tests;
- `tests/` — SCs ontology tests.

### `ostis_ps_user_ecosystem/` — program system

A C++17 CMake project built against `sc-machine`. All agents live in a
single `TrustModule` (`SC_MODULE_REGISTER`) and one shared library
`ostis-ps-trust`, domain-independent:

- `src/agent/` — sc-agent implementations (`ScActionInitiatedAgent`);
- `src/keynodes/` — typed wrappers for sc-elements by domain;
- `src/common/` — reusable sc-memory read/write utilities;
- `src/module/` — `TrustModule` registration;
- `tests/` — GTest tests, including end-to-end integration scenarios;
- `scripts/run_agent.py` — run any agent against a live `sc-server` via
  `py-sc-client`, without rebuilding C++ tests.

### `agents_vault/` — ontological documentation

Obsidian vault: one note per agent (Identification → Purpose →
Competence → Input → Output → Links → Behavior → Security → Metrics →
Tests) plus the index [`Agent Index.md`](agents_vault/Agent%20Index.md)
and an overview of unimplemented candidates
[`Implementation Backlog.md`](agents_vault/Implementation%20Backlog.md).

## Implemented agents

Seventeen sc-agents of the `TrustModule` operationalize rules from the
`concepts/sc_agents.scs` catalog:

| Group | Agents | Rules file | Status |
|---|---|---|---|
| `agent_trust_evaluator` | `AgentUpdateTrustReliability`, `AgentUpdateTrustAccuracy`, `AgentUpdateTrustSecurity`, `AgentUpdateTrustTransparency`, `AgentAggregateTrust` | `rules_trust_update.scs` | complete |
| `agent_cooperative_executor` | `AgentDispatchReadySubtask`, `AgentRegisterPartialResult`, `AgentPropagateToDependent` | `rules_cooperation.scs` | complete |
| `agent_integrity_checker` | `AgentValidateCardinalityConstraint`, `AgentValidateProvenanceChain` | `rules_integrity_validation.scs` | complete (7 of 7 rules) |
| `agent_provenance_registrar` | `AgentComputePiMetric`, `AgentValidateProvenanceChain`* | `rules_provenance_completion.scs` | partial (2 of 6 rules) |
| `agent_result_integrator` | `AgentResolveByTrustWeighted`, `AgentValidateIntegration` | `rules_integration.scs` | partial (2 of 5 rules) |
| `agent_explanation_builder` | `AgentBuildExplanation` | `rules_explanation.scs` | partial (structural core of 4 rules) |
| `agent_coalition_reconfigurator` | `AgentExcludeFromCoalition`, `AgentReassignOnResourceExhaustion` | `rules_coalition_reconfiguration.scs` | partial (2 of 4 rules) |
| security (no single owner) | `AgentDetectAnomalousRequestRate` | `rules_security_privacy.scs` | partial (1 rule) |

\* `AgentValidateProvenanceChain` implements one rule that is duplicated
verbatim in both `rules_integrity_validation.scs`
(`rule_validate_provenance_chain`) and `rules_provenance_completion.scs`
(`rule_verify_transitive_closure`), so it counts toward both groups.

A detailed map of agents, dependencies, and end-to-end tests is in
[`agents_vault/Agent Index.md`](agents_vault/Agent%20Index.md); a
per-rule readiness/status breakdown is in
[`agents_vault/Implementation Backlog.md`](agents_vault/Implementation%20Backlog.md)
and [`ostis_ps_user_ecosystem/AGENTS_AUDIT.md`](ostis_ps_user_ecosystem/AGENTS_AUDIT.md).

## Build and tests

Requires a built [sc-machine](https://github.com/ostis-ai/sc-machine)
side by side (`../sc-machine/build/Release` by default, overridable via
`SC_MACHINE_BUILD_DIR`) and GTest.

```bash
cmake -S ostis_ps_user_ecosystem -B ostis_ps_user_ecosystem/build
cmake --build ostis_ps_user_ecosystem/build
ctest --test-dir ostis_ps_user_ecosystem/build
```

To run an agent against a live `sc-server` without rebuilding tests, use
`ostis_ps_user_ecosystem/scripts/run_agent.py` (see the file docstring).

## Next steps

An overview of the remaining unimplemented agents/rules with a readiness
assessment for each (READY / PARTIAL / NO) is in
[`agents_vault/Implementation Backlog.md`](agents_vault/Implementation%20Backlog.md).
