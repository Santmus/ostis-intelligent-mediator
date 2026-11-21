#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_verify_transitive_closure (rules/rules_provenance_completion.scs),
// дублируемое как rule_validate_provenance_chain
// (rules/rules_integrity_validation.scs): для интегрированного результата
// ir должен существовать transitive_path по nrel_derived_from,
// достигающий экземпляра concept_user_request. Оба правила -- одна и та
// же проверка, реализованная здесь один раз (тот же приём, что и у
// [[AgentValidateIntegration]] для rule_validate_integration/
// rule_validate_integrated_result_shape).
//
// Аргументы sc-действия action_validate_provenance_chain:
//   1) integrated_result -- начальный узел обхода (не обязательно
//                            классифицирован concept_integrated_result --
//                            как и везде в модуле, агент доверяет
//                            вызывающей стороне)
//   2) result_level       -- узел, под которым размещаются индикатор и,
//                            при неполной цепочке, нарушение
//
// nrel_derived_from объявлено транзитивным и антирефлексивным
// (relations/relations_provenance.scs). Множество посещённых узлов
// дополнительно защищает обход от циклов в некорректных данных.
//
// Формальное ядро отношения: concept_result -> concept_result.
// Дополнительно объявлена область concept_user_request для завершающей
// дуги derived_from(result, request), которую требуют это правило и
// affirmation_derived_from_chain. Агент только читает существующие дуги
// и не создаёт новых derived_from.
//
// Результат: valid = ∃ путь nrel_derived_from* от integrated_result до
// узла, классифицированного concept_user_request; записывается как
// 1.0/0.0 в value_link узла result_level (тот же приём, что у
// [[AgentValidateIntegration]]). При invalid дополнительно создаётся
// concept_existence_constraint_violation, присоединяется под result_level
// и связывается с integrated_result через nrel_concerns -- тот же приём,
// что у [[AgentComputePiMetric]]/[[AgentValidateCardinalityConstraint]].
//
// Не реализовано:
//  - "снижение π_data"/"снижение π_proc" -- агент не знает, какой узел
//    metric_level соответствует π_data/π_proc в конкретном вызове, и не
//    пересчитывает их сам; это отдельный вызов [[AgentComputePiMetric]];
//  - запуск rule_fill_missing_provenance -- эвристика без алгоритма (см.
//    её же собственный текст и Limitations [[AgentComputePiMetric]]);
//  - эскалация agent_provenance_registrar -- нет реализованного
//    получателя, как и активация agent_coalition_reconfigurator у
//    [[AgentDetectAnomalousRequestRate]];
//  - ни один агент в проекте пока не создаёт сами дуги nrel_derived_from
//    (используется только в прозе axioms/axioms_provenance.scs, не в
//    сценариях) -- в реальном использовании эта проверка почти всегда
//    вернёт invalid, пока цепочку не начнёт кто-то заполнять;
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentValidateProvenanceChain : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
