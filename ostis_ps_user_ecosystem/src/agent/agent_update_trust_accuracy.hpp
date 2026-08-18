#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_update_t_acc_on_quality
// (ostis_kb_user_ecosystem/rules/rules_trust_update.scs):
//   t_acc(s, tau) <- (1 - beta) * t_acc(s, tau) + beta * q_res
//
// Аргументы sc-действия action_update_trust_accuracy:
//   1) trust_level  -- существующий экземпляр concept_trust_level,
//                      несущий текущее значение t_acc в виде числовой
//                      sc-ссылки, связанной с ним положительной
//                      постоянной дугой (trust_level -> value_link)
//   2) quality_link -- числовая sc-ссылка с достигнутой точностью
//                      результата q_res в диапазоне [0, 1]
//
// Результат: значение t_acc в value_link обновляется на месте; в
// результат действия включаются trust_level и value_link. Если у
// trust_level ещё нет числовой sc-ссылки, она создаётся, а текущее
// значение принимается равным 0 (нейтральный старт).
class AgentUpdateTrustAccuracy : public ScActionInitiatedAgent
{
public:
  static float const ms_beta;

  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
