---
type: agent
tags: [agent, sc-agent, security, privacy, explanation]
action_class: action_validate_right_to_explanation
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentValidateRightToExplanation

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_right_to_explanation`, READY) ·
second agent for `rules_security_privacy.scs`, alongside [[AgentDetectAnomalousRequestRate]]

> [!note] Checks the gate, does not build the explanation or block delivery
> The rule reads "agent_explanation_builder обязан построить..." and "ir
> не может быть выдан пользователю" — this agent does neither. It does
> not call [[AgentBuildExplanation]] when the gate fails, and there is no
> `AgentPublishResult` yet (see [[Agent Index]] §7.5) to actually consume
> this check's indicator before delivery. The agent only reports whether
> the condition holds against already-existing data.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_validate_right_to_explanation` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentValidateRightToExplanation : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_validate_right_to_explanation.{hpp,cpp}` |
| Role | Implements `rule_right_to_explanation` (F_rule_sec, `rules/rules_security_privacy.scs`) |
| Knowledge base rule | `rules/rules_security_privacy.scs` → `rule_right_to_explanation` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `axiom_decision_requires_explanation`
(`axioms/axioms_security.scs`) and `concept_property_right_to_explanation`
(`concepts/security_privacy.scs`) formalize the right to an explanation
of an automated decision, but nothing checked it: an integrated
result/decision could be delivered to a user with no linked explanation
at all, for exactly the request types (decision support, recommendation)
where an explanation is most legally and ethically load-bearing.

**Goal.** Given an integrated result `ir` and the request `q` that
produced it, determine whether the rule's gate applies (`ir` has been
delivered to someone, and `q`'s semantic type requires an explanation)
and, if so, whether `ir` is actually linked to one.

**Value.** Turns a declarative privacy property into an executable,
testable check with a typed violation, the same operationalization
[[AgentValidateIntegration]] gave to `affirmation_formula_7_integrated_result`.

**Stakeholders:** `AgentPublishResult` (not implemented, [[Agent Index]]
§7.5) — the natural caller, which would gate delivery on this
indicator; [[AgentBuildExplanation]] — the natural producer of the
`nrel_explained_by` arc this check reads.

## Competence

**Capabilities:**
- evaluate the two-part gate: `∃ ir -[delivered_to]-> ·` (existence
  only — the specific recipient is irrelevant to the rule's consequent)
  **and** `request` classified `concept_request_decision_support` or
  `concept_request_recommendation`;
- when the gate applies, check `∃ ir -[explained_by]-> ·`
  (`IntegrationKeynodes::nrel_explained_by`, the same relation
  [[AgentValidateIntegration]] checks and [[AgentBuildExplanation]]
  writes);
- when the gate does not apply, report `valid = true` unconditionally —
  the rule imposes no requirement outside those two conditions;
- on invalid, create a violation node classified by **both**
  `concept_integrity_violation` and the already-existing
  `concept_property_right_to_explanation` (`concepts/security_privacy.scs`)
  — the literal pattern of the neighboring rule in the same file,
  `rule_enforce_consent_check` ("создать concept_integrity_violation
  типа concept_property_consent_compliance"). No new ontology class was
  needed, unlike [[AgentCheckAcyclicity]]/[[AgentValidateCardinalityConstraint]];
- write `1.0`/`0.0` into a `value_link` on `result_level` (same
  convention as the rest of the module);
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not invoke `agent_explanation_builder`** — the rule's "обязан
  построить" is not executed; the agent only detects the gap, same
  division of labor as [[AgentValidateIntegration]]/[[AgentValidateProvenanceChain]].
- **does not block delivery** — "ir не может быть выдан пользователю" has
  no enforcement point, since `AgentPublishResult` is not implemented.
- **cannot derive `request` from `integrated_result` itself** — no
  single-hop relation from `ir` back to its originating request exists
  in the ontology, so `request` is a required caller-supplied argument,
  the same design choice as `community` in [[AgentExcludeFromCoalition]].
- **terminology observation, not fixed here:** this rule's text requires
  `explained_by(ir,e)` (`nrel_explained_by`,
  `concept_integrated_result → concept_explanation`), while the
  neighboring *axiom* `axiom_decision_requires_explanation` requires
  `supported_by_explanation(d,e)` (`nrel_supported_by_explanation`,
  `concept_processing_result → concept_explanation`) for the same kind
  of obligation on `concept_collective_decision`. Two different
  relations for a similar requirement — implemented literally against
  the *rule* text (`nrel_explained_by`), since that is what is being
  operationalized here; reconciling it with the axiom's wording is out
  of scope.
- only `IsElement` on arguments, same convention as the rest of the
  module (`request`'s class is read for the gate condition itself, but
  not otherwise validated as a genuine `concept_user_request`).

## Input

**Trigger:** arc `action_initiated -> <instance of action_validate_right_to_explanation>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `integrated_result` | `ScAddr` of the checked integrated result/decision (ir) | required | only `IsElement` |
| 2 | `request` | `ScAddr` of the request q that produced ir | required | only `IsElement`; classification is read for the gate, not enforced |
| 3 | `result_level` | `ScAddr` of the indicator target node | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level`:
`1.0` if the gate does not apply, or applies and `explained_by` is
present; `0.0` if the gate applies and `explained_by` is absent. On
`0.0`, a violation node (classified `concept_integrity_violation` +
`concept_property_right_to_explanation`) is also attached under
`result_level` and linked to `integrated_result` via `nrel_concerns`.
Success — `FinishSuccessfully()` + `FormResult(integrated_result, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`common/relation_io.hpp` (`LinkByRelation`), `keynodes/security_keynodes.hpp`
(`SecurityKeynodes` — `nrel_delivered_to`, `concept_request_decision_support`,
`concept_request_recommendation`, `concept_integrity_violation`,
`concept_property_right_to_explanation` newly added alongside the
existing `concept_security_incident`/`concept_incident_denial_of_service`
from [[AgentDetectAnomalousRequestRate]]), `keynodes/integration_keynodes.hpp`
(`IntegrationKeynodes::nrel_explained_by`, reused from
[[AgentValidateIntegration]]/[[AgentBuildExplanation]]),
`keynodes/integrity_keynodes.hpp` (`IntegrityKeynodes::nrel_concerns`,
reused).

**Reads:** `nrel_delivered_to` arcs from `integrated_result`;
classification of `request`; `nrel_explained_by` arcs from
`integrated_result`.

**Writes:** on gate failure, one violation node + its two classifying
arcs + its `nrel_concerns` arc; the `value_link` of `result_level`.

**Workflow.** Data-independent from every other agent in the module —
reads `nrel_explained_by`, the same relation [[AgentValidateIntegration]]
reads and [[AgentBuildExplanation]] writes, but neither agent is called
by this one, and this one is not called by them.

## Behavior

```
IF  integrated_result is not an sc-element
 OR request is not an sc-element
 OR result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  delivered  := ∃ integrated_result -[delivered_to]-> ·
  requiresExplanation := request classified decision_support OR recommendation
  gateApplies := delivered AND requiresExplanation

  IF gateApplies:
    valid := ∃ integrated_result -[explained_by]-> ·
  ELSE:
    valid := true

  IF NOT valid:
    create violation (ConstNode) classified concept_integrity_violation
                                  AND concept_property_right_to_explanation
    attach violation under result_level
    LinkByRelation(violation, integrated_result, nrel_concerns)

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (valid ? 1.0 : 0.0) to value_link
  action.FormResult(integrated_result, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument; the
gate not applying is a normal, non-error, valid (`1.0`) outcome.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** this agent is itself part of the security/privacy
  subsystem (F_rule_sec), but its own check is only as strong as its
  caller's honesty — nothing prevents a caller from invoking it with a
  `request` that misrepresents the real originating request's type, or
  from ignoring an invalid result and delivering `ir` anyway (no
  enforcement point exists, see Limitations).
- No PII is created or read directly — the checked entities are
  structural (result/request/explanation identity), though the rule
  this operationalizes exists specifically to protect a user-facing
  legal/ethical property (right to explanation of automated decisions).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — one `CreateIterator5` scan for `delivered_to`, two `CheckConnector` calls for the request type, one `CreateIterator5` scan for `explained_by`. |
| Accuracy | Not applicable — deterministic gate/existence check; verified by tests covering both request types that trigger the gate, both gate-bypass conditions (no delivery, wrong request type), and the violation-present case. |
| Cost | Negligible. |
| Alerts | None — the violation is a passive, queryable node; nothing consumes it proactively (no `AgentPublishResult`, see Limitations). |

## Tests

File: `tests/test_agent_validate_right_to_explanation.cpp` (5 tests, all pass).

**1. `ValidWhenGateAppliesAndExplanationPresent`**
Input: `ir` delivered to a user, `request` classified `decision_support`,
`ir` has an `explained_by` arc.
Output: indicator `1.0`, no violation.

**2. `InvalidWhenGateAppliesAndExplanationMissing`**
Input: `ir` delivered, `request` classified `recommendation`, no
`explained_by` arc.
Output: indicator `0.0`; violation created, classified by both
`concept_integrity_violation` and `concept_property_right_to_explanation`,
`nrel_concerns` points to `ir`.

**3. `ValidWhenNotDeliveredRegardlessOfExplanation`**
Input: `request` classified `decision_support`, but `ir` has no
`delivered_to` arc, and no `explained_by` either.
Output: indicator `1.0` — gate does not apply.

**4. `ValidWhenRequestTypeDoesNotRequireExplanation`**
Input: `ir` delivered, but `request` has no `decision_support`/
`recommendation` classification, and no `explained_by` either.
Output: indicator `1.0` — gate does not apply.

**5. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
