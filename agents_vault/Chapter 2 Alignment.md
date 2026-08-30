---
type: backlog
tags: [backlog, alignment, chapter2, trust-agents]
status: pending, not applied
surveyed: 2026-08-25
---

# Chapter 2 Alignment

← [[Agent Index]] · [[Implementation Backlog]]

The implementation was frozen on 2026-08-22 (last agent commit). Chapter 2 of
the dissertation was revised later, on 2026-08-24, by stages 3 and 4 of the
commission review. Two of those revisions reach the code; a third follows from
them. **Nothing here has been applied** — this note records what to change,
where, and why, so the edits can be made in one pass.

Source of truth: `Диссертация_ФИНАЛЬНАЯ/Dissertation/part2.tex` as of
2026-08-24 23:49.

## Scope

Five of the twelve agents are affected: the four EMA agents and
[[AgentAggregateTrust]].

The other seven were checked against the revised chapter and need **no
changes** — recorded here so they are not re-examined:
[[AgentComputePiMetric]], [[AgentDetectAnomalousRequestRate]],
[[AgentResolveByTrustWeighted]], [[AgentValidateCardinalityConstraint]],
[[AgentDispatchReadySubtask]], [[AgentRegisterPartialResult]],
[[AgentPropagateToDependent]]. They either take their parameters from the KB
as action arguments or implement structural graph logic the revisions did not
touch. In particular, the removal of the relevance function ρ (finding S-42)
concerns routing, which is not implemented; the cooperation trio only *reads*
`nrel_assigned_to` and never computes the assignment predicate itself.

## 1. λ weights → 1/4 each — changes computed values

`agent_aggregate_trust.cpp` holds `λ = (0.40, 0.25, 0.15, 0.20)`. These came
from an illustrative calculation in the **previous** redaction of chapter 2;
that illustration has been deleted (verified against
`part2.backup.OSTIS_version.tex`), so the values no longer have a source.

Chapter 2 now sets equal weights as the default (`part2.tex` ~line 2352):
`ω_k = 1/5`, `λ_k = 1/4`, `α_• = 1/5` — chosen "не как удобное, а как
единственное, не вносящее непроверенного предпочтения ни одной компоненте",
with any deviation requiring a stated domain basis formulated *before*
observing results. The same section adds that the remaining fifteen degrees of
freedom "при экспериментальной проверке модели не используются" — λ is among
them.

This matters because the prototype is the experimental apparatus: running it
on unequal weights that the text no longer justifies is the exact fitting the
three fixation rules were written to prevent.

| File | Location | Change |
|---|---|---|
| `src/agent/agent_aggregate_trust.cpp` | 25–28 | four constants → `0.25f` each |
| `src/agent/agent_aggregate_trust.hpp` | 35–43 | comment: drop "иллюстративные веса из part2.tex", state the default-weights rule |
| `tests/test_agent_aggregate_trust.cpp` | 25–29 | recompute the comment |
| `tests/test_agent_aggregate_trust.cpp` | 53 | `0.82345f` → **`0.82975f`** |
| `tests/test_agent_aggregate_trust.cpp` | 84–85 | comment only — the value **stays `0.75f`** (see below) |
| `tests/test_integration_request_pipeline.cpp` | 117 | comment referencing `lambda_rel = 0.40` |
| `tests/test_integration_request_pipeline.cpp` | 118 | `0.04f` → **`0.025f`** |
| `ostis_kb_user_ecosystem/scenarios/scenario_trust_assessment.scs` | 25, 28, 44, 65 | three worked examples |
| `ostis_kb_user_ecosystem/rules/rules_trust_update.scs` | 153–154 | λ description |

Recomputed values:

```
test_agent_aggregate_trust (main)
  t = (0.838, 0.796, 0.955, 0.730)
  old  0.4·0.838 + 0.25·0.796 + 0.15·0.955 + 0.2·0.730 = 0.82345
  new  (0.838 + 0.796 + 0.955 + 0.730) / 4             = 0.82975

test_agent_aggregate_trust (binary case)
  t = (1, 0, 1, 1)
  old  0.4·1 + 0.25·0 + 0.15·1 + 0.2·1 = 0.75
  new  (1 + 0 + 1 + 1) / 4             = 0.75   ← unchanged, comment only

test_integration_request_pipeline
  only t_rel = 0.10 is non-zero
  old  0.40 · 0.10 = 0.04
  new  0.25 · 0.10 = 0.025
  the competitor's trust is 0.01, so 0.025 > 0.01 — the conflict-resolution
  assertion still holds and needs no restructuring

scenario_trust_assessment.scs
  trust_initial  0.806  → (0.82 + 0.78 + 0.95 + 0.70) / 4  = 0.8125
  trust_after    0.823  → (0.838 + 0.796 + 0.955 + 0.730) / 4 = 0.82975
  line 65        0.809  → (0.838 + 0.796 + 0.860 + 0.730) / 4 = 0.806
```

## 2. α/β/γ/δ → single η — naming, no behavioral change

The four EMA agents each declare their own smoothing constant, and all four
are already `0.10f`:

| Agent | Constant | Formula in code |
|---|---|---|
| [[AgentUpdateTrustReliability]] | `ms_alpha` | `(1-α)·t_rel + α·ν_rel` |
| [[AgentUpdateTrustAccuracy]] | `ms_beta` | `(1-β)·t_acc + β·q_res` |
| [[AgentUpdateTrustSecurity]] | `ms_gamma` | `(1-γ)·t_sec + γ·ν_sec` |
| [[AgentUpdateTrustTransparency]] | `ms_delta` | `(1-δ)·t_tr + δ·ν_tr` |

Chapter 2 (`part2.tex` ~262–290, finding S-43) now writes all four equations
with one `η ∈ (0,1)` and argues the point explicitly: distinct adaptation
rates would need a substantive basis separating the inertia of reliability
from that of security, the model gives none, and four independent quantities
instead of one would inflate the free-parameter count "без выигрыша в
описательной силе".

Since all four constants already equal `0.10`, **nothing computes
differently**. But the code literally instantiates the four parameters the
text explains the model does not have — visible to anyone reading both.

Files: the four `agent_update_trust_*.{hpp,cpp}` pairs (constant, formula
comment, header docstring); `agent_detect_anomalous_request_rate.hpp` 9–14,
whose comment cites "у alpha/beta/gamma/delta было явное типично 0.10";
`ostis_kb_user_ecosystem/rules/rules_trust_update.scs` 40–41, 69–70, 96–97,
129–130; and the four agent notes in this vault plus [[Trust Model]].

> [!note] A refinement chapter 2 allows but does not require
> The text names one justified way to split η — asymmetry by sign of the
> observation for `t_sec`, where reaction to an incident should outpace
> recovery, given as a pair `η⁻ > η⁺`. It is to be introduced "только при
> наличии предметного требования". Do not implement it pre-emptively.

## 3. η has to become an action argument

Chapter 2 names `θ`, `w_R` and `η` as the three tunable parameters, calibrated
on a debug set and frozen before the evaluation run
(`part2.tex` ~2356–2372). `η` is currently a compile-time
`static float const`, so calibration would mean recompiling — the procedure
the chapter now commits to is not executable on this prototype.

The correct pattern already exists in the codebase:
[[AgentDetectAnomalousRequestRate]] takes `k_sigma` as an action argument
precisely because the rule text supplied no default. The EMA agents should
follow it.

`θ` and `w_R` belong to routing, which is not implemented — see
[[Implementation Backlog]].

## Sequencing

Item 1 stands alone and is small: six files, four constants, two expected
values. Item 2 and item 3 touch the same files as each other and belong to
**stage 5** of the review protocol (findings S-15, D-7, D-2, D-3 — mapping the
model to SC-code and a machine-verifiable ontology), which is the stage
responsible for KB↔model alignment. Doing 2 and 3 now would mean two passes
over the same headers.
