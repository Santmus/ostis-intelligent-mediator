---
type: agent
tags: [agent, sc-agent, user-adaptation, ontology-extension]
action_class: action_choose_modality
version: (current commit of ostis_ps_user_ecosystem)
status: implemented, covered by tests
---

# AgentChooseModality

← [[Agent Index]] · from [[Implementation Backlog]] (`rule_choose_modality`) ·
second agent for `rules_user_adaptation.scs` (F_rule_adapt), alongside
[[AgentEvaluateProgressiveDisclosure]]

> [!note] This rule required ontology work, not just agent code
> When [[AgentEvaluateProgressiveDisclosure]] was implemented,
> `rule_choose_modality` was re-examined and found NOT actually ready
> despite being labeled READY in [[Implementation Backlog]]: neither of
> its two branch conditions had any typed sc-representation anywhere in
> the KB. Finishing this rule meant formalizing two narrow, targeted
> subclasses first (see Links) — a small, deliberate ontology extension,
> not an invention of new semantics the rule didn't already name.

## Identification

| Field | Value |
|---|---|
| Action class system identifier (sc-IRI) | `action_choose_modality` |
| SC-node class type | `ScType::NodeConstClass` (`sc_node_class`) |
| C++ class | `AgentChooseModality : public ScActionInitiatedAgent` |
| Files | `src/agent/agent_choose_modality.{hpp,cpp}` |
| Role | Implements `rule_choose_modality` (F_rule_adapt, `rules/rules_user_adaptation.scs`) |
| Knowledge base rule | `rules/rules_user_adaptation.scs` → `rule_choose_modality` |
| Module | `TrustModule` (see [[AgentComputePiMetric]] for why) |

## Purpose

**Problem.** `concept_interface_modality`'s six subclasses
(`concepts/adaptive_interface.scs`) were fully typed and ready to
receive a decision, but the rule that decides among them —
`rule_choose_modality` — had no typed inputs to read: neither "the
situational context has a visual perception constraint" nor "the user
prefers visual explanations" existed as anything but prose.

**Goal.** Given a situational context instance and a user model,
formalize the missing typed flags and select the modality accordingly:
voice if visually constrained, visual if preferred and unconstrained,
text otherwise.

**Value.** Closes the last gap in `rules_user_adaptation.scs`'s two
READY-in-practice rules (alongside [[AgentEvaluateProgressiveDisclosure]]),
and — more generally — demonstrates the minimum viable ontology
extension needed to operationalize a rule whose *output* side was ready
but whose *input* side wasn't, without inventing anything the rule text
didn't already name.

**Stakeholders:** whatever future adaptive-interface component would
actually render in the chosen modality (not implemented, same gap as
[[AgentEvaluateProgressiveDisclosure]]'s missing `AgentPublishResult`-class
consumer).

## Competence

**Capabilities:**
- check whether `situation` (a `concept_user_situation_context`
  instance) is additionally classified `concept_visual_perception_constraint`
  (new subclass, `<= nrel_inclusion: concept_user_situation_context`,
  `concepts/contexts.scs`);
- scan `user_model`'s preferences (`nrel_has_preference`) for any
  instance additionally classified `concept_preference_visual_modality`
  (new subclass, `<= nrel_inclusion: concept_user_preference`,
  `concepts/users.scs`);
- select, in the rule's own If/Elif/Default order: visual-constrained →
  `concept_modality_voice`; else prefers-visual → `concept_modality_visual`;
  else → `concept_modality_text` (all three already-existing classes);
- attach the chosen modality class as a `ConstPermPosArc` child of
  `result_level`;
- finish with an error only when an argument is missing.

**Limitations (per code):**
- **result is a class choice, not a numeric indicator** — unlike the
  `AgentValidate*` family, `FormResult(situation, modalityAddr)` returns
  the chosen shared class directly, the same non-numeric-result pattern
  [[AgentBuildExplanation]] established (`FormResult(explanation,
  constructionAct)`); documented explicitly to avoid confusion with the
  usual 1.0/0.0 convention.
- **does not write `nrel_uses_modality`** — that relation's domain is
  `concept_adaptive_interface`, and (as with
  [[AgentEvaluateProgressiveDisclosure]]'s `nrel_applies_strategy`) no
  agent in the project creates such an instance.
- **only the two subclasses this rule needs were formalized** — other
  situational indicators named only in prose elsewhere in the same file
  (`mobile`/`on-the-go` in `rule_choose_presentation_template`,
  `night-shift`/`low-light` in `rule_context_aware_layout`) and other
  preference kinds (e.g. `preference_low_energy` in
  `scenario_request_processing.scs`) remain untyped — deliberately out
  of scope for this specific rule.
- **stops at the first matching preference** — if `user_model` has
  multiple preferences and at least one is `concept_preference_visual_modality`,
  the branch fires; iteration order among preferences is not guaranteed
  by the sc-machine API, but is irrelevant here (existence, not
  identity, is what matters).
- only `IsElement` on arguments; `situation`/`user_model`'s own base
  classes (`concept_user_situation_context`/`concept_user_model`) are
  not verified, only their secondary classification relevant to the
  rule's branches — same convention as the rest of the module.

## Input

**Trigger:** arc `action_initiated -> <instance of action_choose_modality>`.

| # | Role | Type | Required | Validation |
|---|---|---|---|---|
| 1 | `situation` | `ScAddr` of the C_u instance to check for a visual perception constraint | required | only `IsElement` |
| 2 | `user_model` | `ScAddr` of the user model whose preferences are scanned | required | only `IsElement` |
| 3 | `result_level` | `ScAddr` of the node under which the chosen modality is attached | required | only `IsElement` |

## Output

`result_level` gains a `ConstPermPosArc` child pointing at exactly one
of `concept_modality_voice`/`concept_modality_visual`/`concept_modality_text`
(the shared, already-existing classes — not a fresh instance). Success —
`FinishSuccessfully()` + `FormResult(situation, modalityAddr)`.

## Links

**Dependencies:** `keynodes/adaptive_interface_keynodes.hpp`
(`AdaptiveInterfaceKeynodes` — extended with `action_choose_modality`,
`concept_visual_perception_constraint`, `concept_preference_visual_modality`,
`concept_modality_voice`/`_visual`/`_text`, `nrel_has_preference`,
alongside the existing `action_evaluate_progressive_disclosure` from
[[AgentEvaluateProgressiveDisclosure]]).

**Ontology additions (this agent):**
- `concept_visual_perception_constraint` (`concepts/contexts.scs`) —
  `<= nrel_inclusion: concept_user_situation_context`;
- `concept_preference_visual_modality` (`concepts/users.scs`) —
  `<= nrel_inclusion: concept_user_preference`;
- `scenarios/scenario_intelligent_tutoring.scs`'s `preference_ivan_visual`
  updated to also carry `<- concept_preference_visual_modality` — it
  already named itself "visual" in free text and in its own system
  identifier; this makes that intent a typed fact instead of prose.

**Reads:** secondary classification of `situation`; `nrel_has_preference`
arcs from `user_model` and secondary classification of each target.

**Writes:** one `ConstPermPosArc` from `result_level` to the chosen
modality class.

**Workflow.** Data-independent from every other agent in the module,
including [[AgentEvaluateProgressiveDisclosure]] — both are agents of
`rules_user_adaptation.scs` but read/write disjoint data.

## Behavior

```
IF  situation is not an sc-element
 OR user_model is not an sc-element
 OR result_level is not an sc-element
THEN  log error → action.FinishWithError()  [STOP]

ELSE
  IF situation classified concept_visual_perception_constraint:
    modality := concept_modality_voice
  ELIF ∃ preference IN user_model.nrel_has_preference
       WHERE preference classified concept_preference_visual_modality:
    modality := concept_modality_visual
  ELSE:
    modality := concept_modality_text

  attach result_level -[ConstPermPosArc]-> modality
  action.FormResult(situation, modality)
  → action.FinishSuccessfully()
```

**Error handling:** the only path to an error is a missing argument;
every other input combination resolves to exactly one modality, by
construction (the `ELSE` branch is exhaustive).

**Timeouts:** as with the rest of the module — no internal timeout,
`DoProgram` is synchronous.

## Security

- Privileges — same as other `TrustModule` agents: no ACL, full access
  to sc-memory within the `sc-server` process.
- **Specific risk:** none beyond the general one — purely a
  classification read plus one arc write; no PII is read or written
  directly (preference/situation *identity* is structural, though the
  underlying real-world fact — a visual impairment or accessibility
  need — is sensitive; this agent never sees or stores anything beyond
  the already-existing typed flag).

## Metrics

| Metric | Value / status |
|---|---|
| Latency | Synchronous, no I/O; O(1) for the situation check, O(N) in the number of the user model's preferences for the preference scan. |
| Accuracy | Not applicable — deterministic three-way branch; verified by tests covering each branch independently and the priority order when both conditions hold. |
| Cost | Negligible. |
| Alerts | None — the result is a passive classification; nothing consumes it proactively (no adaptive-interface renderer exists yet, see Limitations). |

## Tests

File: `tests/test_agent_choose_modality.cpp` (5 tests, all pass).

**1. `ChoosesVoiceWhenVisualPerceptionConstrained`**
Input: `situation` classified `concept_visual_perception_constraint`,
`user_model` also has a `concept_preference_visual_modality` preference.
Output: `concept_modality_voice` attached, `concept_modality_visual` NOT
attached — confirms the constraint branch takes priority.

**2. `ChoosesVisualWhenPreferenceVisualPresent`**
Input: `situation` unconstrained, `user_model` has one unrelated
preference and one `concept_preference_visual_modality` preference.
Output: `concept_modality_visual` attached.

**3. `DefaultsToTextWhenNoConditionApplies`**
Input: `situation` unconstrained, `user_model`'s one preference is not
`concept_preference_visual_modality`.
Output: `concept_modality_text` attached.

**4. `VisualConstraintTakesPriorityOverPreference`**
Input: both conditions hold simultaneously.
Output: `concept_modality_voice` attached, `concept_modality_visual` NOT
— explicit priority-order test, distinct from test 1's implicit check.

**5. `FinishesWithErrorOnMissingArguments`**
Input: action without arguments.
Output: `FinishedWithError`.
