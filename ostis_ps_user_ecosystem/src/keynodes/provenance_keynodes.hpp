#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агентов провенансной подсистемы
// (rules/rules_provenance_completion.scs): вычисление метрик полноты
// провенанса (rule_compute_pi_metrics), проверка цепочки трассировки
// (rule_verify_transitive_closure, дублируется как
// rule_validate_provenance_chain в rules/rules_integrity_validation.scs)
// и автоматическая регистрация PROV-O триады (rule_auto_record_prov_entity,
// rule_auto_record_prov_activity). nrel_has_provenance/nrel_derived_from/
// concept_user_request/concept_prov_*/nrel_prov_*/rrel_actor/rrel_input/
// rrel_timestamp -- уже существующие элементы онтологии
// (relations/relations_provenance.scs, concepts/interactions.scs,
// concepts/provenance.scs, relations/role_relations.scs); системные
// идентификаторы совпадают, поэтому ScKeynode находит существующие
// узлы. action_compute_pi_metric, action_validate_provenance_chain,
// action_record_prov_entity и action_record_prov_activity также
// объявлены в SC-коде (rules/rules_provenance_completion.scs) для
// документации и единообразия модели.
class ProvenanceKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_has_provenance{"nrel_has_provenance", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_derived_from{"nrel_derived_from", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_user_request{"concept_user_request", ScType::NodeConstClass};

  // PROV-O триада (concepts/provenance.scs) -- rule_auto_record_prov_entity/
  // rule_auto_record_prov_activity.
  static inline ScKeynode const concept_prov_entity{"concept_prov_entity", ScType::NodeConstClass};
  static inline ScKeynode const concept_prov_activity{"concept_prov_activity", ScType::NodeConstClass};

  static inline ScKeynode const nrel_prov_was_generated_by{
      "nrel_prov_was_generated_by", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_prov_was_attributed_to{
      "nrel_prov_was_attributed_to", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_prov_used{"nrel_prov_used", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_prov_was_associated_with{
      "nrel_prov_was_associated_with", ScType::ConstNodeNonRole};

  static inline ScKeynode const rrel_actor{"rrel_actor", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_input{"rrel_input", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_timestamp{"rrel_timestamp", ScType::ConstNodeRole};

  static inline ScKeynode const action_compute_pi_metric{"action_compute_pi_metric", ScType::NodeConstClass};
  static inline ScKeynode const action_validate_provenance_chain{
      "action_validate_provenance_chain", ScType::NodeConstClass};
  static inline ScKeynode const action_record_prov_entity{
      "action_record_prov_entity", ScType::NodeConstClass};
  static inline ScKeynode const action_record_prov_activity{
      "action_record_prov_activity", ScType::NodeConstClass};
};
