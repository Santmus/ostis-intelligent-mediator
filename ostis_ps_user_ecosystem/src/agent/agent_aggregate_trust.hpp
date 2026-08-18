#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_aggregate_trust
// (ostis_kb_user_ecosystem/rules/rules_trust_update.scs):
//   trust(s, tau, h) = lambda_rel * t_rel + lambda_acc * t_acc
//                     + lambda_sec * t_sec + lambda_tr * t_tr
//
// В отличие от AgentUpdateTrust* (экспоненциальное сглаживание одного
// индикатора), это не накопление во времени, а мгновенный пересчёт
// скалярной оценки из уже посчитанных компонент -- поэтому результат
// каждый раз перезаписывается заново, а не смешивается со старым
// значением.
//
// Аргументы sc-действия action_aggregate_trust (все -- существующие
// экземпляры concept_trust_level с числовой sc-ссылкой значения,
// связанной положительной постоянной дугой trust_level -> value_link;
// компонента без ссылки трактуется как нейтральный старт t_k = 0,
// как и в AgentUpdateTrust*):
//   1) t_rel_level -- надёжность t_rel
//   2) t_acc_level -- достоверность t_acc
//   3) t_sec_level -- безопасность t_sec
//   4) t_tr_level  -- прозрачность t_tr
//   5) trust_level -- узел для агрегированной оценки trust(s, tau, h);
//                     если у него ещё нет числовой sc-ссылки, она
//                     создаётся
//
// Результат: значение trust в value_link узла trust_level (5-й
// аргумент) обновляется на месте; в результат действия включаются
// trust_level и value_link.
class AgentAggregateTrust : public ScActionInitiatedAgent
{
public:
  // Иллюстративные веса из part2.tex / scenario_trust_assessment.scs
  // (лямбда экспертно заданы, Σ = 1). Модель предполагает и обучаемые
  // веса ("либо обучаются" в тексте правила), но подбор/обновление
  // весов по обратной связи -- отдельная, пока не реализованная,
  // задача; здесь используется фиксированный экспертный вариант.
  static float const ms_lambdaRel;
  static float const ms_lambdaAcc;
  static float const ms_lambdaSec;
  static float const ms_lambdaTr;

  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
