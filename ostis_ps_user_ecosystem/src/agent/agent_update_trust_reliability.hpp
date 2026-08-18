#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_update_t_rel_on_outcome
// (ostis_kb_user_ecosystem/rules/rules_trust_update.scs):
//   t_rel(s, tau) <- (1 - alpha) * t_rel(s, tau) + alpha * nu_rel
//
// Аргументы sc-действия action_update_trust_reliability:
//   1) trust_level  -- существующий экземпляр concept_trust_level,
//                      несущий текущее значение t_rel в виде числовой
//                      sc-ссылки, связанной с ним положительной
//                      постоянной дугой (trust_level -> value_link)
//   2) outcome_link -- числовая sc-ссылка с индикатором исхода
//                      nu_rel в диапазоне [0, 1]
//
// Результат: значение t_rel в value_link обновляется на месте; в
// результат действия включаются trust_level и value_link. Если у
// trust_level ещё нет числовой sc-ссылки, она создаётся, а текущее
// значение принимается равным 0 (нейтральный старт).
class AgentUpdateTrustReliability : public ScActionInitiatedAgent
{
public:
  static float const ms_alpha;

  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
