#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агентов rules/rules_security_privacy.scs:
// детектирования аномальной частоты запросов
// (rule_detect_anomalous_request_rate) и проверки права на объяснение
// (rule_right_to_explanation). Все перечисленные ниже -- уже
// существующие элементы онтологии (concepts/security_privacy.scs,
// STRIDE-классификация; relations/relations_execution.scs;
// concepts/interactions.scs; concepts/integrity.scs); системные
// идентификаторы совпадают, поэтому ScKeynode находит существующие
// узлы, а не создаёт дубликаты. action_detect_anomalous_request_rate и
// action_validate_right_to_explanation также объявлены в SC-коде для
// документации и единообразия модели.
class SecurityKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const concept_security_incident{"concept_security_incident", ScType::NodeConstClass};

  // STRIDE-подтип, выбираемый этим агентом (AGENTS_AUDIT.md п.6.9):
  // сигнал агента -- чисто объёмный (rate/baseline), у него нет
  // доступа к содержимому запросов, поэтому он может обосновать только
  // Denial of Service (аномальный объём), но не Tampering (подделка
  // данных, требующая анализа содержимого) -- тем самым правило
  // "DoS либо Tampering" (rules_security_privacy.scs) для этого
  // конкретного агента разрешается однозначно.
  static inline ScKeynode const concept_incident_denial_of_service{
      "concept_incident_denial_of_service", ScType::NodeConstClass};

  static inline ScKeynode const action_detect_anomalous_request_rate{
      "action_detect_anomalous_request_rate", ScType::NodeConstClass};

  // rule_right_to_explanation: delivered_to(ir,u) -- условие гейта
  // (relations/relations_execution.scs, concept_processing_result ->
  // concept_user).
  static inline ScKeynode const nrel_delivered_to{"nrel_delivered_to", ScType::ConstNodeNonRole};

  // request_type(q) ∈ {decision_support, recommendation} -- семантический
  // тип запроса (concepts/interactions.scs, подклассы concept_user_request).
  static inline ScKeynode const concept_request_decision_support{
      "concept_request_decision_support", ScType::NodeConstClass};
  static inline ScKeynode const concept_request_recommendation{
      "concept_request_recommendation", ScType::NodeConstClass};

  // "нарушение concept_property_right_to_explanation" -- по буквальному
  // образцу соседнего правила того же файла (rule_enforce_consent_check:
  // "создать concept_integrity_violation типа
  // concept_property_consent_compliance"): узел нарушения
  // классифицируется ОБОИМИ классами сразу -- общим
  // concept_integrity_violation (concepts/integrity.scs, единая
  // иерархия нарушений во всём проекте) и специфическим
  // concept_property_right_to_explanation (concepts/security_privacy.scs,
  // уже существующий класс -- новый подкласс не потребовался, в отличие
  // от AgentCheckAcyclicity/AgentValidateCardinalityConstraint).
  static inline ScKeynode const concept_integrity_violation{"concept_integrity_violation", ScType::NodeConstClass};
  static inline ScKeynode const concept_property_right_to_explanation{
      "concept_property_right_to_explanation", ScType::NodeConstClass};

  static inline ScKeynode const action_validate_right_to_explanation{
      "action_validate_right_to_explanation", ScType::NodeConstClass};
};
