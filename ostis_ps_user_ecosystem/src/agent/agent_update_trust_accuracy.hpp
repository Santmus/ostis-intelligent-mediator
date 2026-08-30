#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_update_t_acc_on_quality
// (ostis_kb_user_ecosystem/rules/rules_trust_update.scs):
//   t_acc(s, tau) <- (1 - eta) * t_acc(s, tau) + eta * q_res
//
// Аргументы sc-действия action_update_trust_accuracy:
//   1) trust_level  -- существующий экземпляр concept_trust_level,
//                      несущий текущее значение t_acc в виде числовой
//                      sc-ссылки, связанной с ним положительной
//                      постоянной дугой (trust_level -> value_link)
//   2) quality_link -- числовая sc-ссылка с достигнутой точностью
//                      результата q_res в диапазоне [0, 1]
//   3) eta_link     -- необязательная числовая sc-ссылка с коэффициентом
//                      сглаживания eta в (0, 1); если не передана,
//                      используется ms_eta (0.10). eta -- один из трёх
//                      калибруемых параметров модели (theta, w_R, eta)
//                      пересмотренной главы 2, поэтому передаётся
//                      аргументом, а не зашивается в код (образец --
//                      k_sigma в AgentDetectAnomalousRequestRate)
//
// Результат: значение t_acc в value_link обновляется на месте; в
// результат действия включаются trust_level и value_link. Если у
// trust_level ещё нет числовой sc-ссылки, она создаётся, а текущее
// значение принимается равным 0 (нейтральный старт).
class AgentUpdateTrustAccuracy : public ScActionInitiatedAgent
{
public:
  // Значение eta по умолчанию; единый коэффициент сглаживания для всех
  // четырёх EMA-агентов.
  static float const ms_eta;

  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
