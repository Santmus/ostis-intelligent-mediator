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
// Правило также требует "если pi < 1.0 -- создать concept_integrity_violation
// типа transparency_gap; запустить rule_fill_missing_provenance"
// (AGENTS_AUDIT.md п.6.7). Первая часть реализована: при непустой группе
// и pi < 1.0 создаётся экземпляр concept_existence_constraint_violation
// (concepts/integrity.scs) -- "transparency_gap" в онтологии нет как
// формального класса, только как неструктурированный текст правила,
// поэтому используется уже существующий подкласс concept_integrity_violation,
// семантически ближайший к "ожидаемая связь отсутствует"; нарушение
// подвешено под metric_level и связано с проверяемой elements_group
// через nrel_concerns (тот же приём, что и в
// AgentValidateCardinalityConstraint). Пустая группа нарушения не
// создаёт -- измерять нечего, а не "всё потеряно".
//
// Не реализовано: запуск rule_fill_missing_provenance (восстановление
// пропущенного провенанса) и фиксация причины невозможности
// восстановления -- по AGENTS_AUDIT.md §6.7 это зона ответственности
// отдельных agent_provenance_recovery/agent_provenance_registrar,
// не реализованных в проекте (см. §7 аудита).
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
