---
type: agent
tags: [agent, sc-agent, security, threshold]
action_class: action_detect_anomalous_request_rate
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests, verified on a live sc-server
---

# AgentDetectAnomalousRequestRate

← [[Agent Index]] · from [[Implementation Backlog]] (top-5, #2)

> [!note] k_sigma is an argument, not a constant
> Unlike the EMA agents' `η` (explicitly “typically 0.10” in the rule text
> of [[AgentUpdateTrustReliability|trust agents]], hence an *optional*
> argument there with default `0.10`), k_sigma has no default
> textual value — only the “27×” observation in the illustrative scenario,
> which describes a *specific incident*, not a threshold. Therefore, here
> k_sigma is the third argument of the action; the calling side decides
> what to consider anomalous for the given component.

## Identification

| Field | Value |
|---|---|
| System identifier (sc-IRI) of the action class | `action_detect_anomalous_request_rate` |
| sc-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentDetectAnomalousRequestRate : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_detect_anomalous_request_rate.{hpp,cpp}` |
| Role | Implements the computable part of `rule_detect_anomalous_request_rate` (part of the F_rule_sec security subsystem, with no single dedicated “owner” agent in the `concepts/sc_agents.scs` catalog) |
| KB rule | `rules/rules_security_privacy.scs` → `rule_detect_anomalous_request_rate` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] — same point about one module per `.dylib`) |

## Purpose

**Problem.** A spike in request frequency to an ecosystem component can
indicate a DoS attack or a brute-force attempt (`concept_incident_denial_of_service`/
`concept_incident_tampering` in `concepts/security_privacy.scs`) — but
“many requests” by itself means nothing without comparison to the normal
load of the specific component.

**Goal.** Compare the observed frequency with the baseline multiplied by
a threshold coefficient, and if an excess is detected, record it as an
indicator and create an instance of `concept_security_incident`.

**Value.** An automatic, reproducible (not “by eye”) answer to “is this
already an incident or still normal load” — the first step of the
response chain described in `scenario_security_breach_response.scs`.

**Stakeholders:** ecosystem security officer (`user_security_officer`
in the scenario), `agent_coalition_reconfigurator` (not implemented —
would be expected to react to the incident), users of the attacked
component.

## Competence

**Capabilities:**
- from three numbers (`rate`, `baseline`, `k_sigma`) compute whether the
  baseline is exceeded by the given margin;
- write a boolean indicator (1.0/0.0) into the target node’s sc-link,
  creating it on first access;
- on anomaly detection, generate an instance of `concept_security_incident`,
  also classified `concept_incident_denial_of_service`, and link it to
  the target node;
- terminate with an error when arguments are missing or content is
  non-numeric.

**Limitations (per code):**
- **the STRIDE subclass choice is a deliberate narrowing, not a
  general solution** — the rule text leaves "DoS либо Tampering"
  unformalized, but this agent's *only* signal is a volumetric ratio
  (`rate`/`baseline`); it never sees request content, so it cannot ever
  justify Tampering (a data-integrity violation). Classifying as
  `concept_incident_denial_of_service` is therefore sound for this
  specific agent (AGENTS_AUDIT.md §6.9), but does not generalize to a
  rule that could plausibly indicate either subclass.
- **does not activate** `agent_coalition_reconfigurator` and
  `agent_provenance_registrar` “to record the incident,” as the rule text
  requires — both are outside this agent’s scope (the first is not
  implemented at all; [[AgentComputePiMetric]] implements only part of
  the second, and not in a form that could be called directly from here).
- the incident → target_node link is **not a named ontology relation**
  but a plain `ConstPermPosArc` for traceability (similar to how
  `trust_level` stores its `value_link`); there is no formal link between
  the incident and the specific `target_component` that was attacked —
  the ontology has no ready-made relation for this.
- the remaining AGENTS_AUDIT.md §6.9 bullets are still open: the
  incident is not linked to a subject/system/request/observation
  period (the agent doesn’t even receive their identities as
  arguments — only pre-computed `rate`/`baseline` numbers), gets no
  audit record (`concept_audit_record`’s own definition ties it to an
  execution act or a failure, not to a security incident — reusing it
  here would need a new relation the audit does not name), does not
  lower `t_sec`, and does not pause a route, trigger an access check,
  a reassignment, or an escalation. None of these has a concrete
  relation name in the rule text or the audit (unlike, say,
  `is_input_for` for §6.10), so closing them would mean inventing new
  ontology rather than reusing existing vocabulary — deferred pending
  that design decision.
- `k_sigma` is nowhere validated as > 0 — a negative `k_sigma` will make
  almost any non-zero traffic “anomalous”.
- `baseline <= 0` is a special case (see Behavior): division by zero is
  generally undefined, but the agent treats it as a deterministic decision
  rather than an error, which may mask an invalid input baseline.

## Input

**Trigger:** an `action_initiated` arc to <an `action_detect_anomalous_request_rate` instance>.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `rate_link` | sc-link with observed frequency | required | `IsElement` + parses with `std::stof` |
| 2 | `baseline_link` | sc-link with baseline | required | `IsElement` + parses with `std::stof` |
| 3 | `k_sigma_link` | sc-link with threshold multiplier | required | `IsElement` + parses with `std::stof` |
| 4 | `result_level` | indicator target node | required | `IsElement` only |

## Output

The updated/created sc-link `value_link` on `result_level` with `1.0`
(anomaly) or `0.0` (normal). If an anomaly — a `concept_security_incident`,
also classified `concept_incident_denial_of_service`, is additionally
generated and linked to `result_level` (`ConstPermPosArc`).
Success — `FinishSuccessfully()` + `FormResult(resultLevel, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`ReadFloatLinkContent`/
`WriteFloatLinkContent`), `keynodes/security_keynodes.hpp`
(`SecurityKeynodes::concept_security_incident`,
`concept_incident_denial_of_service`, `action_detect_anomalous_request_rate`).

**Reads:** the content of the three numeric sc-links.

**Writes:** `value_link` of `result_level`; on anomaly — creates a new
node of class `concept_security_incident`.

**Workflow.** Independent of the other five agents; conceptually feeds
the unimplemented `agent_coalition_reconfigurator` (expected to react to
the created incident) — connection is data-only
(`concept_security_incident` in shared sc-memory), not a direct call.

## Behavior

```
IF  any of the four arguments is not an sc-element
THEN  FinishWithError()

ELSE IF  rate/baseline/k_sigma do not parse as numbers
THEN  FinishWithError()

ELSE
  IF  baseline > 0
  THEN  ratio := rate / baseline
  ELSE IF  rate > 0
  THEN  ratio := +infinity        // traffic from nowhere -- anomaly
  ELSE  ratio := 0                // no traffic and no history -- not anomaly

  anomalous := ratio > k_sigma

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (anomalous ? 1.0 : 0.0) to value_link

  IF  anomalous
  THEN  create node of class concept_security_incident,
        also classified concept_incident_denial_of_service,
        link result_level -> incident (ConstPermPosArc)

  action.FormResult(result_level, value_link)
  → action.FinishSuccessfully()
```

Illustrative example: `rate=5400, baseline=200` → `ratio=27`; at
`k_sigma=5` — `27 > 5` → anomaly, incident created. The value “27”
literally matches “exceeds the baseline by 27 times” from
`scenario_security_breach_response.scs`.

**Error handling/timeouts:** same as the other agents in the project —
`DoProgram` is synchronous, waiting is on the caller side.

## Security

- Privileges — same as the other agents of `TrustModule`: no ACL, full
  access to sc-memory within the `sc-server` process.
- **Specific risk:** the fact of “baseline exceedance” is computed
  correctly, but an understated `baseline` or `k_sigma` passed by the
  caller will cause false positives (or misses) without any plausibility
  check by the agent — trust in the parameters lies entirely with the
  caller.
- The created `concept_security_incident` is essentially a public
  statement of a security incident; the agent does not verify the
  caller’s right to make such a statement (see the project-wide
  Limitation — no authentication of the action initiator).
- Committing to `concept_incident_denial_of_service` specifically (vs.
  the generic base class before this fix) means a downstream consumer
  now sees a confident STRIDE classification even though the underlying
  signal is only ever "rate exceeded baseline by some factor" — a
  legitimate but unusual traffic spike gets the same specific label as
  a genuine attack.
- The data (request frequencies) are abstract numbers and contain no PII.

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O — comparable to the 2-argument trust agents (constant number of operations, independent of input size). |
| Accuracy | Not applicable to the formula itself; correctness is verified by tests, including exact reproduction of the illustrative example (27×). |
| Cost | Negligible. |
| Alerts | Not implemented in the agent itself (see Limitations — activation of `agent_coalition_reconfigurator` is not performed), but `concept_security_incident` remains in sc-memory as a persistent, queryable signal for external monitoring. |

## Tests

File: `tests/test_agent_detect_anomalous_request_rate.cpp` (3 tests, all
pass; scenario 1 additionally verified on a live sc-server — got
`1.000000` + `concept_security_incident` created).

**1. `DetectsAnomalyAndCreatesIncident`**
Input: `rate=5400, baseline=200, k_sigma=5` (5400/200=27, “exceeds the
baseline by 27 times” from `scenario_security_breach_response.scs`).
Output: indicator `1.0`, a node of class `concept_security_incident`
created and also classified `concept_incident_denial_of_service`
(AGENTS_AUDIT.md §6.9 — a volumetric signal can only justify DoS),
`FinishedSuccessfully`.

**2. `DoesNotFlagNormalTraffic`**
Input: `rate=250, baseline=200, k_sigma=5` (250/200=1.25 ≤ 5).
Output: indicator `0.0`, incident not created.

**3. `FinishesWithErrorOnMissingArguments`**
Input: action with no arguments.
Output: `FinishedWithError`.
