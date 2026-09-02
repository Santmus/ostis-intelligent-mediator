#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агентов rules/rules_user_adaptation.scs
// (F_rule_adapt) -- rule_progressive_disclosure, rule_choose_modality и
// rule_context_aware_layout. concept_visual_perception_constraint,
// concept_night_shift_context, concept_low_light_context
// (concepts/contexts.scs) и concept_preference_visual_modality
// (concepts/users.scs) -- новые подклассы-флаги, формализованные вместе
// с AgentChooseModality/AgentEvaluateContextAwareLayout (см. их
// собственные комментарии в KB): условия ветвления обоих правил прежде
// существовали только в прозе. concept_modality_text/_voice/_visual и
// concept_strategy_context_aware_layout (concepts/adaptive_interface.scs)
// и nrel_has_preference (relations/relations_intentional.scs) -- уже
// существующие элементы онтологии.
class AdaptiveInterfaceKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const action_evaluate_progressive_disclosure{
      "action_evaluate_progressive_disclosure", ScType::NodeConstClass};
  static inline ScKeynode const action_choose_modality{"action_choose_modality", ScType::NodeConstClass};
  static inline ScKeynode const action_evaluate_context_aware_layout{
      "action_evaluate_context_aware_layout", ScType::NodeConstClass};

  static inline ScKeynode const concept_visual_perception_constraint{
      "concept_visual_perception_constraint", ScType::NodeConstClass};
  static inline ScKeynode const concept_preference_visual_modality{
      "concept_preference_visual_modality", ScType::NodeConstClass};
  static inline ScKeynode const concept_night_shift_context{
      "concept_night_shift_context", ScType::NodeConstClass};
  static inline ScKeynode const concept_low_light_context{"concept_low_light_context", ScType::NodeConstClass};

  static inline ScKeynode const concept_modality_voice{"concept_modality_voice", ScType::NodeConstClass};
  static inline ScKeynode const concept_modality_visual{"concept_modality_visual", ScType::NodeConstClass};
  static inline ScKeynode const concept_modality_text{"concept_modality_text", ScType::NodeConstClass};
  static inline ScKeynode const concept_strategy_context_aware_layout{
      "concept_strategy_context_aware_layout", ScType::NodeConstClass};

  static inline ScKeynode const nrel_has_preference{"nrel_has_preference", ScType::ConstNodeNonRole};
};
