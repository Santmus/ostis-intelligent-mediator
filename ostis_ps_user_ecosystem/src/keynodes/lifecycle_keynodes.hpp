#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для AgentRecordStateTransition
// (rules/rules_mediation_orchestration.scs, action_record_state_transition):
// учёт перехода состояния запроса в конвейере медиации
// (concepts/mediation_lifecycle.scs, concepts/processing_plan.scs).
// Все элементы -- уже существующие узлы онтологии, системные
// идентификаторы совпадают, поэтому ScKeynode находит существующие узлы;
// новых элементов эта Фаза 1 не вводит, кроме самого
// action_record_state_transition.
//
// Фаза 1 ограничена уже покрытым агентами участком конвейера --
// x_exec/x_aggregate/x_verify плюс ребро переназначения x_exec->x_assign
// (AGENTS_AUDIT.md §7.1, план Фазы 1) -- поэтому среди классов состояний
// перечислены только concept_state_execution/_assignment/_aggregation/
// _verification. Остальные 9 классов состояний (concept_state_accepted,
// _interpretation, _planning, _replanning, _presentation, _partial,
// _clarification, _refusal, _cancelled) относятся к Фазе 2 (агенты
// §7.5, которых сейчас нет) и здесь не нужны.
class LifecycleKeynodes : public ScKeynodes
{
public:
  // Нероле-вые отношения (concepts/mediation_lifecycle.scs,
  // concepts/processing_plan.scs, relations/relations_provenance.scs).
  static inline ScKeynode const nrel_has_state{"nrel_has_state", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_transitions_to{"nrel_transitions_to", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_caused_by_event{"nrel_caused_by_event", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_uses_plan_version{"nrel_uses_plan_version", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_transition_recorded_in{
      "nrel_transition_recorded_in", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_remaining_reassignment_budget{
      "nrel_remaining_reassignment_budget", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_remaining_replanning_depth{
      "nrel_remaining_replanning_depth", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_has_provenance{"nrel_has_provenance", ScType::ConstNodeNonRole};

  // Ролевые отношения реификации concept_state_transition
  // (rrel_transition_actor/rrel_context осознанно не включены -- вне
  // минимального обязательного пакета constraint_transition_provenance,
  // см. план Фазы 1).
  static inline ScKeynode const rrel_from_state{"rrel_from_state", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_to_state{"rrel_to_state", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_transition_event{"rrel_transition_event", ScType::ConstNodeRole};

  // Классы.
  static inline ScKeynode const concept_user_request{"concept_user_request", ScType::NodeConstClass};
  static inline ScKeynode const concept_mediation_state{"concept_mediation_state", ScType::NodeConstClass};
  static inline ScKeynode const concept_state_transition{"concept_state_transition", ScType::NodeConstClass};
  static inline ScKeynode const concept_plan_version{"concept_plan_version", ScType::NodeConstClass};
  static inline ScKeynode const concept_audit_record{"concept_audit_record", ScType::NodeConstClass};
  static inline ScKeynode const concept_audit_record_transition{
      "concept_audit_record_transition", ScType::NodeConstClass};
  static inline ScKeynode const concept_provenance_record{"concept_provenance_record", ScType::NodeConstClass};

  // Классы состояний в рамках Фазы 1 (см. комментарий выше).
  static inline ScKeynode const concept_state_execution{"concept_state_execution", ScType::NodeConstClass};
  static inline ScKeynode const concept_state_assignment{"concept_state_assignment", ScType::NodeConstClass};
  static inline ScKeynode const concept_state_aggregation{"concept_state_aggregation", ScType::NodeConstClass};
  static inline ScKeynode const concept_state_verification{"concept_state_verification", ScType::NodeConstClass};

  static inline ScKeynode const action_record_state_transition{
      "action_record_state_transition", ScType::NodeConstClass};
};
