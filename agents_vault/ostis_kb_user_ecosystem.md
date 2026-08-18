---
type: repo
tags: [repo, knowledge-base]
---

# ostis_kb_user_ecosystem

Каталог с онтологией на SCs (не входит в этот vault — рядом:
`../ostis_kb_user_ecosystem/`). Ключевые для агентов файлы:

- `rules/rules_trust_update.scs` — все пять правил π_tr и классы
  действий (`action_update_trust_*`, `action_aggregate_trust`).
- `concepts/trust.scs` — модель [[Модель доверия]] и `concept_trust_level`.
- `concepts/security_privacy.scs` — `concept_security_incident`,
  STRIDE-классификация (контекст для [[AgentUpdateTrustSecurity]]).
- `scenarios/scenario_trust_assessment.scs` — иллюстративный расчёт,
  воспроизведённый в тестах [[AgentUpdateTrustReliability]] и
  [[AgentAggregateTrust]].

Симлинкован в `ostis-web-platform/knowledge-base/ostis_kb_user_ecosystem`
для сборки БЗ через `sc-builder`.
