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
// nrel_derived_from объявлено транзитивным (relations/relations_provenance.scs),
// но НЕ антирефлексивным -- обход выполняется с множеством посещённых
// узлов, чтобы цикл в графе (в т.ч. образованный некорректными данными)
// не привёл к бесконечному циклу, а не потому что циклы ожидаются в
// нормальном случае.
//
// Наблюдение по домену отношения (не исправляется здесь): nrel_derived_from
// объявлено как concept_processing_result -> concept_processing_result
// (relations/relations_provenance.scs), тогда как concept_user_request --
// подкласс concept_interaction_entity, а не concept_processing_result
// (concepts/interactions.scs). Формально дуга derived_from(result, request)
// выходит за объявленный домен отношения, но именно такую цепочку
// требует и текст этого правила, и affirmation_derived_from_chain
// (axioms/axioms_provenance.scs: "восстановления полного пути формирования
// любого результата от исходного запроса"). Агент только читает уже
// существующие дуги и не создаёт новых derived_from -- вопрос
// согласования домена с формулировкой правила его не касается.
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
