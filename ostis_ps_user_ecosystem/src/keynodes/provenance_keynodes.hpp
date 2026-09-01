#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агентов провенансной подсистемы
// (rules/rules_provenance_completion.scs): вычисление метрик полноты
// провенанса (rule_compute_pi_metrics) и проверка цепочки трассировки
// (rule_verify_transitive_closure, дублируется как
// rule_validate_provenance_chain в rules/rules_integrity_validation.scs).
// nrel_has_provenance/nrel_derived_from/concept_user_request -- уже
// существующие элементы онтологии (relations/relations_provenance.scs,
// concepts/interactions.scs); системные идентификаторы совпадают,
// поэтому ScKeynode находит существующие узлы. action_compute_pi_metric
// и action_validate_provenance_chain также объявлены в SC-коде
// (rules/rules_provenance_completion.scs) для документации и
// единообразия модели.
class ProvenanceKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_has_provenance{"nrel_has_provenance", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_derived_from{"nrel_derived_from", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_user_request{"concept_user_request", ScType::NodeConstClass};

  static inline ScKeynode const action_compute_pi_metric{"action_compute_pi_metric", ScType::NodeConstClass};
  static inline ScKeynode const action_validate_provenance_chain{
      "action_validate_provenance_chain", ScType::NodeConstClass};
};
