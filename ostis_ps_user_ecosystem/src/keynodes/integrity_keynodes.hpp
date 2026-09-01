#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента проверки кардинальных ограничений
// SHACL-подобных shape'ов (rules/rules_integrity_validation.scs,
// concepts/integrity.scs). concept_cardinality_constraint_violation и
// concept_existence_constraint_violation были только в прозе
// (nrel_sc_text_translation) шести правил, но не объявлены как
// sc_node_class -- формализованы вместе с этим агентом. Три
// concept_cardinality_check_* -- новые маркеры направления/типа связи:
// сами SHACL-shape'ы различают "property X", "property X⁻¹" и
// "property rrel_Y" только текстом, без формального признака.
//
// concept_existence_constraint_violation и nrel_concerns также
// переиспользуются AgentComputePiMetric (AGENTS_AUDIT.md п.6.7) --
// это тот же тип нарушения "ожидаемая связь отсутствует", только
// проверяемая сущность там -- группа измеряемых элементов, а не
// произвольный target.
class IntegrityKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const concept_cardinality_constraint_violation{
      "concept_cardinality_constraint_violation", ScType::NodeConstClass};
  static inline ScKeynode const concept_existence_constraint_violation{
      "concept_existence_constraint_violation", ScType::NodeConstClass};

  static inline ScKeynode const concept_cardinality_check_outgoing_non_role{
      "concept_cardinality_check_outgoing_non_role", ScType::NodeConstClass};
  static inline ScKeynode const concept_cardinality_check_incoming_non_role{
      "concept_cardinality_check_incoming_non_role", ScType::NodeConstClass};
  static inline ScKeynode const concept_cardinality_check_outgoing_role{
      "concept_cardinality_check_outgoing_role", ScType::NodeConstClass};

  // concerns(violation, target) -- типизированная связь нарушения
  // с проверяемой сущностью (concepts/integrity.scs, AGENTS_AUDIT.md
  // п.6.8: прежде нарушение оставалось анонимным узлом без указания,
  // что именно оно нарушает).
  static inline ScKeynode const nrel_concerns{"nrel_concerns", ScType::ConstNodeNonRole};

  static inline ScKeynode const action_validate_cardinality_constraint{
      "action_validate_cardinality_constraint", ScType::NodeConstClass};
};
