#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует вычислимую часть rule_progressive_disclosure
// (rules/rules_user_adaptation.scs, F_rule_adapt): если глубина
// provenance-цепочки объяснения > 4 шагов ИЛИ количество частичных
// результатов > 5, требуется прогрессивное раскрытие (executive
// summary с возможностью развёртывания по требованию) вместо полного
// представления.
//
// Аргументы sc-действия action_evaluate_progressive_disclosure:
//   1) explanation           -- узел, от которого измеряется глубина
//                                цепочки nrel_derived_from (агент не
//                                проверяет его класс -- тот же приём,
//                                что у AgentValidateProvenanceChain:
//                                nrel_derived_from формально определено
//                                для concept_result и завершающего
//                                concept_user_request, но не для
//                                concept_explanation; агент доверяет
//                                вызывающей стороне)
//   2) partial_results_group -- узел-контейнер, чьи ДЕТИ (дуги
//                                ConstPermPosArc) -- считаемые частичные
//                                результаты; agent сам их не отбирает
//                                и не проверяет класс -- тот же приём
//                                elements_group, что у
//                                AgentComputePiMetric
//   3) result_level           -- узел, под которым размещается индикатор
//
// ВАЖНО: индикатор здесь -- не "валидно/невалидно" (как у
// Agent Validate*), а сама булева величина решения правила: 1.0 =
// прогрессивное раскрытие требуется (сработало хотя бы одно из двух
// условий), 0.0 = не требуется. Это единственный агент модуля, где
// 1.0 означает "правило сработало", а не "нарушений нет" -- явно
// зафиксировано здесь, чтобы не создавать ложной аналогии с
// AgentValidateIntegration/AgentValidateProvenanceChain/AgentCheckAcyclicity.
//
// Глубина -- длина САМОЙ ДЛИННОЙ цепочки исходящих дуг nrel_derived_from
// от explanation (0 для узла без исходящих derived_from), вычисляется
// мемоизированным DFS с защитой от цикла (узел, уже находящийся на
// текущем пути обхода, не обходится повторно и не даёт бесконечной
// рекурсии) -- тот же класс защиты, что у AgentCheckAcyclicity/
// AgentValidateProvenanceChain, применённый здесь не к обнаружению
// цикла, а к безопасному вычислению глубины при его случайном наличии.
//
// Не реализовано:
//  - "адаптивный интерфейс представляет верхний уровень" -- ни один
//    агент проекта не создаёт экземпляр concept_adaptive_interface, и
//    этот агент тоже не принимает его аргументом и не пишет
//    nrel_applies_strategy(interface, concept_strategy_progressive_disclosure)
//    -- материализация решения средствами интерфейса выходит за рамки
//    backend-модуля sc-агентов;
//  - "с возможностью развёртывания каждого шага" -- UI-механика,
//    не онтологический факт;
//  - только IsElement на аргументах, как и у остальных агентов модуля;
//    partial_results_group интерпретируется буквально как "любые дети
//    через ConstPermPosArc", без проверки, что это действительно
//    concept_partial_result (тот же уровень доверия к вызывающей
//    стороне, что у AgentComputePiMetric).
class AgentEvaluateProgressiveDisclosure : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
