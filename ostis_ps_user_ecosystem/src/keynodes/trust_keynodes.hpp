#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы, используемые агентами обновления компонент
// доверия (t_rel, t_acc, ...). concept_trust_level — уже существующий
// класс онтологии ostis_kb_user_ecosystem (concepts/trust.scs), общий
// для всех компонент вектора доверия; системный идентификатор
// совпадает, поэтому ScKeynode находит существующий узел, а не создаёт
// дубликат. action_update_trust_* — классы действий, также объявленные
// в SC-коде (rules/rules_trust_update.scs) для документации и
// единообразия модели.
class TrustKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const concept_trust_level{"concept_trust_level", ScType::NodeConstClass};

  static inline ScKeynode const action_update_trust_reliability{
      "action_update_trust_reliability", ScType::NodeConstClass};

  static inline ScKeynode const action_update_trust_accuracy{
      "action_update_trust_accuracy", ScType::NodeConstClass};

  static inline ScKeynode const action_update_trust_security{
      "action_update_trust_security", ScType::NodeConstClass};

  static inline ScKeynode const action_update_trust_transparency{
      "action_update_trust_transparency", ScType::NodeConstClass};

  static inline ScKeynode const action_aggregate_trust{"action_aggregate_trust", ScType::NodeConstClass};
};
