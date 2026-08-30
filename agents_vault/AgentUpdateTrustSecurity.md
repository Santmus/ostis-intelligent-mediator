---
type: agent
tags: [agent, trust-agent, sc-agent, ema, security]
action_class: action_update_trust_security
version: 72ba122 (2026-08-18)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentUpdateTrustSecurity

← [[Agent Index]] · component of the [[Trust Model|t_sec]] · same pattern as [[AgentUpdateTrustReliability]]

> [!warning] `ms_gamma` → `η` pending — see [[Chapter 2 Alignment]]
> Chapter 2 as revised 2026-08-24 uses a single smoothing coefficient `η` for
> all four components instead of α/β/γ/δ. The value is unchanged (`0.10`), so
> nothing computes differently; the rename is a naming fix, and `η` should also
> become an action argument so it can be calibrated without recompiling.
> Not yet applied — everything below describes the current code.
>
> This is the one component for which the chapter allows η to be split: a pair
> `η⁻ > η⁺`, so that reaction to an incident outpaces recovery from it. It is to
> be introduced "только при наличии предметного требования" — do **not**
> implement it pre-emptively.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_update_trust_security` |
| Type of sc-node class | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentUpdateTrustSecurity : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_update_trust_security.{hpp,cpp}` |
| Role | Executor of rule `rule_update_t_sec_on_incident` (π_tr) |
| KB rule | `rules/rules_trust_update.scs` → `rule_update_t_sec_on_incident` |
| Version | commit `72ba122` |

## Purpose

**Problem.** A system can be reliable and accurate while still violating
security policies or becoming a source of incidents (see the STRIDE
classification in `concepts/security_privacy.scs`:
Spoofing/Tampering/Repudiation/Information Disclosure/DoS/EoP). This is a
separate dimension of trust — `t_sec`.

**Goal.** After each processed subtask, record whether a security incident was
registered (`ν_sec = 0`) or not (`ν_sec = 1`), and update `t_sec` using the
same EMA pattern as the other components.

**Value.** The "immediate drop on incident at the next update step" mentioned
in the rule text is not a separate special logic; it is a direct consequence of
the formula when `ν_sec = 0`: the model self-calibrates, reducing the
probability of assigning subtasks to a system that has committed a violation,
without manual intervention (see `affirmation_trust_self_calibration` in
`concepts/trust.scs`).

**Stakeholders:** security/compliance officers, task orchestrators with
heightened data-protection requirements (see `nrel_processes_personal_data`,
`concept_access_control_policy` in `concepts/security_privacy.scs`), end users
whose data are processed by `s`.

## Competence

**Capabilities:**
- For a pair (`trust_level` node, sc-link with `ν_sec`) compute the
  exponentially smoothed new value of `t_sec`;
- Create an sc-link on first access (neutral start `t_sec = 0` — note
  explicitly: a "new system" starts at `t_sec = 0`, not from a "presumption of
  security" of 1; this is a model-wide choice, but it is especially noticeable
  for `t_sec`);
- Return the updated `trust_level`/`value_link`;
- Terminate correctly with an error on incorrect arguments.

**Limitations (per code):**
- Same basic limitations as [[AgentUpdateTrustReliability]] (no `trust_level`
  class check, no range check, no provenance, `γ = 0.10` is a constant);
- **Key difference from a "smart" security agent:** this code **does not detect
  or classify incidents** itself — it only applies the formula to the already
  prepared indicator `ν_sec`, which an external mechanism must compute (e.g.,
  matching against `concept_security_incident`/`nrel_security_violation_of` in
  `security_privacy.scs`). Registering a specific incident, as mentioned in the
  rule text, is the responsibility of the caller, not of this agent.
- Consequently, if the external detector is wrong or stale, `t_sec` will be
  updated from an incorrect signal, and the agent will neither check nor notice
  it.

## Input

**Trigger:** arc `action_initiated -> <instance of action_update_trust_security>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `trust_level` | `ScAddr` | required | `IsElement` only |
| 2 | `incident_link` | `ScAddr` of an sc-link with `ν_sec` ∈ {0,1} | required | `IsElement` + parsed with `std::stof` |

**Format:** `incident_link` is a textual sc-link, `"1.0"` (no incident) or
`"0.0"` (incident registered).

## Output

The updated/created sc-link `value_link` of `trust_level`. Side effect:
generating `value_link` + arc on first call. Success: `FinishSuccessfully()` +
`FormResult(trustLevel, valueLink)`.

## Links

Identical to the other three EMA agents: `common/float_link_io.hpp`,
`TrustKeynodes`. Conceptually related to the security ontology
(`concept_security_incident` and STRIDE classes in `security_privacy.scs`), but
**not programmatically integrated** with it — this is a purely numeric handler.
The result is read by [[AgentAggregateTrust]].

## Behavior

```
IF  trust_level is not an sc-element  OR  incident_link is not an sc-element
THEN  FinishWithError()

ELSE IF  content of incident_link is not a number
THEN  FinishWithError()

ELSE
  currentTSec := value of value_link if present, ELSE 0 (+ create if absent)
  newTSec := (1 - γ) * currentTSec + γ * ν_sec       // γ = 0.10
  write newTSec, FormResult(...), FinishSuccessfully()
```

Example of immediate drop: `t_sec = 0.955`, incident (`ν_sec = 0`) →
`t_sec_new = 0.9 · 0.955 + 0.1 · 0 ≈ 0.860` in one step.

Error handling/timeouts — same as [[AgentUpdateTrustReliability]].

## Security

- Process privileges — same as all agents in the module (no ACL, full access
  to sc-memory).
- **Agent-specific risk:** because it does not verify the nature of the
  "incident," a false-positive or false-negative external detector will directly
  and without additional checks distort the trust component that is responsible
  specifically for security — i.e., a bug in *another* component of the ecosystem
  can silently undermine trust in an honest system (or, conversely, hide a real
  incident).
- The `ν_sec` itself is an abstract indicator and contains no PII; but if
  `incident_link` is later extended with incident details, those details must not
  be placed in the textual content of the sc-link without a separate storage
  policy.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O — same as the other EMA agents. |
| Accuracy | Not applicable to the formula itself; but **depends on the accuracy of the external incident detector**, which this agent does not measure. |
| Cost | Negligible. |
| Alerts | Not implemented. For `t_sec` they are especially important — a sharp drop should be raised as a signal to a security officer, not only silently factored into trust. |

## Tests

File: `tests/test_agent_update_trust_security.cpp` (3 tests, all passing;
scenario 1 additionally verified on a live sc-server).

**1. `DecreasesTSecOnIncident`**
Input: `t_sec = 0.95`, `ν_sec = 0` (incident).
Output: `t_sec_new = 0.9·0.95 + 0.1·0 = 0.855`, `FinishedSuccessfully`.

**2. `CreatesValueLinkWhenMissing`**
Input: `trust_level` without an sc-link, `ν_sec = 1`.
Output: a link is created with value `0.9·0 + 0.1·1 = 0.10`.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
