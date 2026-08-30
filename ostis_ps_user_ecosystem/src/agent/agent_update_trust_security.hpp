#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_update_t_sec_on_incident
// (ostis_kb_user_ecosystem/rules/rules_trust_update.scs):
//   t_sec(s, tau) <- (1 - eta) * t_sec(s, tau) + eta * nu_sec
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
//   3) eta_link        -- необязательная числовая sc-ссылка с коэффициентом
//                         сглаживания eta в (0, 1); если не передана,
//                         используется ms_eta (0.10). eta -- один из трёх
//                         калибруемых параметров модели (theta, w_R, eta)
//                         пересмотренной главы 2, поэтому передаётся
//                         аргументом, а не зашивается в код (образец --
//                         k_sigma в AgentDetectAnomalousRequestRate)
//
// Результат: значение t_sec в value_link обновляется на месте; в
// результат действия включаются trust_level и value_link. Если у
// trust_level ещё нет числовой sc-ссылки, она создаётся, а текущее
// значение принимается равным 0 (нейтральный старт).
class AgentUpdateTrustSecurity : public ScActionInitiatedAgent
{
public:
  // Значение eta по умолчанию; единый коэффициент сглаживания для всех
  // четырёх EMA-агентов. Допустимое главой 2 разделение eta на
  // eta- > eta+ по знаку наблюдения для t_sec пока сознательно не
  // реализовано (вводится только при наличии предметного требования).
  static float const ms_eta;

  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
