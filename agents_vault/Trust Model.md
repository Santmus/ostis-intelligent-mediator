---
type: concept
tags: [concept, trust-model]
---

# Trust Model T(s, τ)

Source: `ostis_kb_user_ecosystem/concepts/trust.scs`.

```
T(s, τ) = ⟨t_rel(s,τ), t_acc(s,τ), t_sec(s,τ), t_tr(s,τ)⟩
trust(s, τ, h) = λ_rel·t_rel + λ_acc·t_acc + λ_sec·t_sec + λ_tr·t_tr,  Σλ = 1
```

> [!note] Aligned with chapter 2 as revised 2026-08-24 on 2026-08-30
> Both changes applied: the default weights are `λ_k = 1/4` (the code
> previously carried `(0.40, 0.25, 0.15, 0.20)` from a deleted
> illustration), and the four per-component smoothing coefficients
> α/β/γ/δ are a single `η`, passed to the EMA agents as an optional
> action argument with default `0.10`.

- `s` — an intelligent system in the community.
- `τ` — context/type of the task.
- `h` — interaction history used to incrementally update the components.
- Each component ∈ [0, 1] *by ontology design*; none of the agents
  checks this range programmatically (see Limitations in the agents’
  notes).

## Components

| Symbol | Note | Updated by |
|---|---|---|
| t_rel | [[AgentUpdateTrustReliability]] | interaction success indicator ν_rel |
| t_acc | [[AgentUpdateTrustAccuracy]] | achieved result accuracy q_res |
| t_sec | [[AgentUpdateTrustSecurity]] | absence-of-incident indicator ν_sec |
| t_tr | [[AgentUpdateTrustTransparency]] | justification-provided indicator ν_tr |
| trust | [[AgentAggregateTrust]] | weighted sum of the four components above |

## Storage in sc-memory

A component is represented by a container node `trust_level` (an instance
of `concept_trust_level`) to which a numeric sc-link is attached by a
positive permanent arc (`ConstPermPosArc`) carrying the value:

```
concept_trust_level -> trust_level;;
trust_level -> value_link;;      // value_link = sc-link, content = "0.838000"
```

The value is stored as a **text** string (not raw float bytes) — see the
explanation in `common/float_link_io.hpp`, motivated by the
incompatibility of the `SetLinkContent`/`GetLinkContent` serialization
paths for numbers in sc-machine. The same `concept_trust_level` is used
as a common container for any component; which particular component is
stored in a given `trust_level` is determined from context (the node
identifier), not from the sc-element type: the code has no separate class
for each component.

## Purpose in the system

`trust(s, τ, h)` is used by the executor selection function `match(σ, s)`
when distributing subtasks among the intelligent systems of the
community — the model self-tunes: systematic successes increase trust,
while systematic failures/incidents decrease it, automatically reducing
the probability that a subtask is assigned to an unreliable system
without external intervention.
