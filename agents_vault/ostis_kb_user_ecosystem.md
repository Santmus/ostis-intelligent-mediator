---
type: repo
tags: [repo, knowledge-base]
---

# ostis_kb_user_ecosystem

Directory with the ontology in SCs (not inside this vault — next to it:
`../ostis_kb_user_ecosystem/`). Files that are key for the agents:

- `rules/rules_trust_update.scs` — all five π_tr rules and action classes
  (`action_update_trust_*`, `action_aggregate_trust`).
- `concepts/trust.scs` — the [[Trust Model]] and `concept_trust_level`.
- `concepts/security_privacy.scs` — `concept_security_incident`,
  STRIDE classification (context for [[AgentUpdateTrustSecurity]]).
- `scenarios/scenario_trust_assessment.scs` — illustrative calculation,
  reproduced in tests for [[AgentUpdateTrustReliability]] and
  [[AgentAggregateTrust]].

Symlinked into `ostis-web-platform/knowledge-base/ostis_kb_user_ecosystem`
for KB assembly via `sc-builder`.
