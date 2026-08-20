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

  static inline ScKeynode const action_validate_cardinality_constraint{
      "action_validate_cardinality_constraint", ScType::NodeConstClass};
};
