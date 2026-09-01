#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента построения объяснения
// (concepts/explanations.scs, rules/rules_explanation.scs). Реализует
// структурную реификацию акта построения объяснения по шаблону
// prov_template_explanation (axioms/axioms_provenance.scs) и сценарию
// scenarios/scenario_explanation_construction.scs -- роли rrel_explanandum,
// rrel_explanation_type, rrel_uses_results, rrel_applies_rules,
// rrel_based_on, rrel_construction_time уже существуют в KB.
// Не включает связывание использованных правил/источников через
// nrel_prov_used: его домен (concept_prov_activity -> concept_prov_entity,
// concepts/provenance.scs) не установлен для узлов explanation/rule/source
// -- см. Limitations agent_build_explanation.hpp.
class ExplanationKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_supported_by_explanation{
      "nrel_supported_by_explanation", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_explained_by{"nrel_explained_by", ScType::ConstNodeNonRole};

  static inline ScKeynode const rrel_explanandum{"rrel_explanandum", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_explanation_type{"rrel_explanation_type", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_uses_results{"rrel_uses_results", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_applies_rules{"rrel_applies_rules", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_based_on{"rrel_based_on", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_construction_time{"rrel_construction_time", ScType::ConstNodeRole};

  static inline ScKeynode const concept_explanation_construction_act{
      "concept_explanation_construction_act", ScType::NodeConstClass};
  static inline ScKeynode const concept_integrated_result{"concept_integrated_result", ScType::NodeConstClass};

  static inline ScKeynode const action_build_explanation{"action_build_explanation", ScType::NodeConstClass};
};
