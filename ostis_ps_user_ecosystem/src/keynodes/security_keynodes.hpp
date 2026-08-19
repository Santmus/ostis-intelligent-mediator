#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента детектирования аномальной частоты
// запросов (rule_detect_anomalous_request_rate,
// rules/rules_security_privacy.scs). concept_security_incident -- уже
// существующий класс онтологии (concepts/security_privacy.scs);
// системный идентификатор совпадает, поэтому ScKeynode находит
// существующий узел. action_detect_anomalous_request_rate также
// объявлен в SC-коде для документации и единообразия модели.
class SecurityKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const concept_security_incident{"concept_security_incident", ScType::NodeConstClass};

  static inline ScKeynode const action_detect_anomalous_request_rate{
      "action_detect_anomalous_request_rate", ScType::NodeConstClass};
};
