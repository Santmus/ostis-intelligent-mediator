#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует часть правила rule_compute_pi_metrics
// (ostis_kb_user_ecosystem/rules/rules_provenance_completion.scs):
//   pi = |{x in elements_group : has_provenance(x)}| / |elements_group|
//
// Правило описывает три метрики -- pi_proc, pi_dec, pi_data -- как один
// и тот же расчёт "доля элементов с провенансом" над тремя разными
// множествами (процессы/решения/данные). Онтология не даёт единого
// формального класса для этих трёх множеств (в отличие от
// concept_trust_level, общего для всех компонент доверия), поэтому
// агент реализован обобщённо: какое именно множество измеряется
// (Proc, Dec или Data) определяется тем, какую группу элементов передал
// вызывающий код -- один и тот же C++ агент вычисляет любую из трёх
// метрик в зависимости от аргумента.
//
// Не реализована часть правила "если pi < 1.0 -- создать
// concept_integrity_violation типа transparency_gap; запустить
// rule_fill_missing_provenance": в онтологии нет формально определённого
// класса/отношения для типизации нарушения как именно "transparency_gap"
// (только неструктурированный текст правила), а rule_fill_missing_provenance
// сама по себе -- эвристика без алгоритма (см. agents_vault/Кандидаты
// на реализацию.md). Агент ограничен вычислимой частью правила.
//
// Аргументы sc-действия action_compute_pi_metric:
//   1) elements_group -- узел с исходящими положительными постоянными
//                        дугами (ConstPermPosArc) к каждому измеряемому
//                        элементу (процессу/решению/единице данных)
//   2) metric_level    -- узел для итоговой метрики; если у него ещё
//                        нет числовой sc-ссылки, она создаётся
//
// Результат: значение pi в value_link (2-й аргумент) обновляется на
// месте; в результат действия включаются metric_level и value_link.
// Пустая группа (elements_group без элементов) трактуется как pi = 0,
// а не как ошибка -- см. Limitations в agents_vault.
class AgentComputePiMetric : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
