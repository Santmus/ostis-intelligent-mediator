#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента детектирования аномальной частоты
// запросов (rule_detect_anomalous_request_rate,
// rules/rules_security_privacy.scs). concept_security_incident и
// concept_incident_denial_of_service -- уже существующие классы
// онтологии (concepts/security_privacy.scs, STRIDE-классификация);
// системные идентификаторы совпадают, поэтому ScKeynode находит
// существующие узлы. action_detect_anomalous_request_rate также
// объявлен в SC-коде для документации и единообразия модели.
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
};
