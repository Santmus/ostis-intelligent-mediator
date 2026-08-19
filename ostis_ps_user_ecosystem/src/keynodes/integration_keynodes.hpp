#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента разрешения конфликта результатов по
// взвешенному доверию (rule_resolve_by_trust_weighted,
// rules/rules_integration.scs). Все перечисленные ниже -- уже
// существующие элементы онтологии (relations/relations_execution.scs,
// relations/relations_trust.scs, concepts/interactions.scs); системные
// идентификаторы совпадают, поэтому ScKeynode находит существующие
// узлы, а не создаёт дубликаты. action_resolve_by_trust_weighted также
// объявлен в SC-коде для документации и единообразия модели.
class IntegrationKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_produced_by{"nrel_produced_by", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_has_trust_level{"nrel_has_trust_level", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_integrated_in{"nrel_integrated_in", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_resolves_conflict{"nrel_resolves_conflict", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_integrated_result{"concept_integrated_result", ScType::NodeConstClass};

  static inline ScKeynode const action_resolve_by_trust_weighted{
      "action_resolve_by_trust_weighted", ScType::NodeConstClass};
};
