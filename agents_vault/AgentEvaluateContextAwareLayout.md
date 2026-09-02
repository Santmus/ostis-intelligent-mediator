---
type: agent
tags: [agent, sc-agent, user-adaptation, ontology-extension]
action_class: action_evaluate_context_aware_layout
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentEvaluateContextAwareLayout

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_context_aware_layout`) ·
third agent for `rules_user_adaptation.scs` (F_rule_adapt), alongside
[[AgentEvaluateProgressiveDisclosure]] and [[AgentChooseModality]]

> [!note] Closes the gate, not the whole consequent
> Unlike `rule_choose_modality` (where every branch of the consequent
> was fully materializable once the input flags existed),
> `rule_context_aware_layout`'s consequent has a genuinely unquantified
> clause — "снизить количество одновременно показываемых элементов"
> (reduce the number of simultaneously shown elements) has no target
> count anywhere in the text, the same class of gap as
> `cognitive_load(u)`. This agent closes the *detection* — which
> situational condition triggers the strategy — and materializes the
> triggered case as a single existing marker class, per its own
> Limitations. It does not compute or apply the reduction, the dark
> theme, or the font scaling individually.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_evaluate_context_aware_layout` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentEvaluateContextAwareLayout : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_evaluate_context_aware_layout.{hpp,cpp}` |
| Role | Implements the mechanical part of `rule_context_aware_layout` (F_rule_adapt, `rules/rules_user_adaptation.scs`) |
| Knowledge base rule | `rules/rules_user_adaptation.scs` → `rule_context_aware_layout` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `concept_strategy_context_aware_layout`
(`concepts/adaptive_interface.scs`) named the strategy since before this
implementation pass, but neither of its two trigger conditions
("night-shift", "low-light") had any typed sc-representation — the same
class of gap [[AgentChooseModality]] closed for `rule_choose_modality`,
found by the same method (re-examining a PARTIAL rule's actual KB state
rather than trusting the backlog's category label at face value).

**Goal.** Given a situational context instance, detect whether either
trigger condition holds and, if so, mark that the context-aware-layout
strategy applies.

**Value.** Gives `concept_strategy_context_aware_layout` — previously an
inert, disconnected class — its first real producer, and closes the
detection half of the last rule in `rules_user_adaptation.scs` whose
gate condition was a narrow, nameable typing gap rather than an
open-ended formula.

**Stakeholders:** whatever future adaptive-interface rendering component
would read this marker to actually apply dark theme / reduce element
count / scale fonts — none exists yet (same gap as
[[AgentEvaluateProgressiveDisclosure]]/[[AgentChooseModality]]).

## Competence

**Capabilities:**
- check whether `situation` (a `concept_user_situation_context`
  instance) is classified `concept_night_shift_context` **or**
  `concept_low_light_context` (two new subclasses, both
  `<= nrel_inclusion: concept_user_situation_context`,
  `concepts/contexts.scs` — declared as two separate flags because the
  rule names them as two distinct conditions, not one merged concept,
  the same discipline [[AgentChooseModality]] applied);
- when triggered, attach the already-existing
  `concept_strategy_context_aware_layout` class as a `ConstPermPosArc`
  child of `result_level` — a single marker standing in for the whole
  consequent package, since none of its three individual effects (dark
  theme, element-count reduction, font scaling) has its own typed
  representation to attach separately;
- write `1.0`/`0.0` into a `value_link` on `result_level` (decision
  polarity, matching [[AgentEvaluateProgressiveDisclosure]] — `1.0` =
  "the rule fired", not "no violation");
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **does not reduce the element count** — "снизить количество... элементов"
  names no target quantity anywhere in the text (agents_vault/[[Implementation Backlog]]),
  the same open-ended-formula class of gap as `cognitive_load(u)` in
  `rule_choose_presentation_template`.
- **does not apply or compute the font scaling** — the ×1.10 formula
  itself has no ambiguity, but there is nothing in the KB representing
  "current font size" to multiply; applying it needs a live UI state
  this backend module has no access to.
- **does not separate the three consequent effects** — dark theme,
  element reduction, and font scaling are all folded into one marker
  attachment (`concept_strategy_context_aware_layout`) rather than
  three independent facts, since only that one class has any typed
  representation.
- **does not write `nrel_applies_strategy`** — no agent in the project
  creates a `concept_adaptive_interface` instance, the same gap already
  documented for [[AgentEvaluateProgressiveDisclosure]]/[[AgentChooseModality]].
- only `IsElement` on arguments; `situation`'s base class
  (`concept_user_situation_context`) is not verified, only its
  secondary classification relevant to the rule's gate — same
  convention as the rest of the module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_evaluate_context_aware_layout>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `situation` | `ScAddr` of the C_u instance to check for night-shift/low-light | required | only `IsElement` |
| 2 | `result_level` | `ScAddr` of the node under which the indicator and (if triggered) the strategy marker are attached | required | only `IsElement` |

## Output

The updated/created numeric sc-link `value_link` on `result_level`:
`1.0` if `situation` is classified `concept_night_shift_context` or
`concept_low_light_context`, `0.0` otherwise. On `1.0`,
`concept_strategy_context_aware_layout` is also attached as a
`ConstPermPosArc` child of `result_level`. Success —
`FinishSuccessfully()` + `FormResult(situation, valueLink)`.

## Links

**Dependencies:** `common/float_link_io.hpp` (`WriteFloatLinkContent`),
`keynodes/adaptive_interface_keynodes.hpp` (`AdaptiveInterfaceKeynodes` —
extended with `action_evaluate_context_aware_layout`,
`concept_night_shift_context`, `concept_low_light_context`,
`concept_strategy_context_aware_layout`, alongside the existing entries
from [[AgentEvaluateProgressiveDisclosure]]/[[AgentChooseModality]]).

**Reads:** secondary classification of `situation`.

**Writes:** on trigger, one `ConstPermPosArc` from `result_level` to
`concept_strategy_context_aware_layout`; the `value_link` of
`result_level`.

**Workflow.** Data-independent from every other agent in the module,
including its two `rules_user_adaptation.scs` siblings — all three read
and write disjoint data.

## Behavior

```
IF  situation is not an sc-element  OR  result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  triggered := situation classified concept_night_shift_context
               OR situation classified concept_low_light_context

  IF triggered:
    attach result_level -[ConstPermPosArc]-> concept_strategy_context_aware_layout

  IF  result_level has no value_link
  THEN  create value_link (ConstNodeLink) + arc

  write (triggered ? 1.0 : 0.0) to value_link
  action.FormResult(situation, value_link)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument;
neither trigger condition holding is a normal, non-error, `0.0` outcome.

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — a pure classification
  read plus one marker attachment; no PII is read or written (the
  underlying real-world signal — time of day, ambient light — is
  accessibility-adjacent but this agent never sees raw sensor data,
  only the already-classified flag).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) — two `CheckConnector` calls. |
| Accuracy | Not applicable — deterministic two-way OR; verified by tests covering each trigger independently and the not-triggered case. |
| Cost | Negligible. |
| Alerts | None — the marker is passive; nothing consumes it proactively (no adaptive-interface renderer exists yet, see Limitations). |

## Tests

File: `tests/test_agent_evaluate_context_aware_layout.cpp` (4 tests, all pass).

**1. `TriggersOnNightShift`**
Input: `situation` classified `concept_night_shift_context`.
Output: indicator `1.0`; `concept_strategy_context_aware_layout` attached.

**2. `TriggersOnLowLight`**
Input: `situation` classified `concept_low_light_context`.
Output: indicator `1.0`; strategy attached — confirms the second,
independent branch of the OR condition.

**3. `DoesNotTriggerWithoutEitherFlag`**
Input: `situation` with neither classification.
Output: indicator `0.0`; no strategy attached.

**4. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
