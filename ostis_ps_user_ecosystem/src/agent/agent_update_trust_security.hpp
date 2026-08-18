#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_update_t_sec_on_incident
// (ostis_kb_user_ecosystem/rules/rules_trust_update.scs):
//   t_sec(s, tau) <- (1 - gamma) * t_sec(s, tau) + gamma * nu_sec
//
// Аргументы sc-действия action_update_trust_security:
//   1) trust_level     -- существующий экземпляр concept_trust_level,
//                         несущий текущее значение t_sec в виде числовой
//                         sc-ссылки, связанной с ним положительной
//                         постоянной дугой (trust_level -> value_link)
//   2) incident_link    -- числовая sc-ссылка с индикатором отсутствия
//                         инцидента безопасности nu_sec в {0, 1}
//                         (0 -- зарегистрирован инцидент, 1 -- инцидентов
//                         не было); регистрация конкретного инцидента
//                         (concept_security_incident, см.
//                         security_privacy.scs) -- забота вызывающего
//                         сценария, агент лишь применяет формулу к
//                         переданному индикатору
//
// Результат: значение t_sec в value_link обновляется на месте; в
// результат действия включаются trust_level и value_link. Если у
// trust_level ещё нет числовой sc-ссылки, она создаётся, а текущее
// значение принимается равным 0 (нейтральный старт).
class AgentUpdateTrustSecurity : public ScActionInitiatedAgent
{
public:
  static float const ms_gamma;

  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
