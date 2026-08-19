#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента вычисления метрик полноты провенанса
// (rule_compute_pi_metrics, rules/rules_provenance_completion.scs).
// nrel_has_provenance -- уже существующее отношение онтологии
// (relations/relations_provenance.scs), домен -- concept_processing_result,
// область значений -- concept_provenance_record; системный идентификатор
// совпадает, поэтому ScKeynode находит существующий узел. action_compute_pi_metric
// также объявлен в SC-коде (rules/rules_provenance_completion.scs) для
// документации и единообразия модели.
class ProvenanceKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_has_provenance{"nrel_has_provenance", ScType::ConstNodeNonRole};

  static inline ScKeynode const action_compute_pi_metric{"action_compute_pi_metric", ScType::NodeConstClass};
};
