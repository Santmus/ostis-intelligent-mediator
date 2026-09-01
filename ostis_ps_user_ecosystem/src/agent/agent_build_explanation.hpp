#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует структурную (реификационную) часть правил построения
// объяснения rule_build_local_explanation / rule_build_global_explanation /
// rule_build_contrastive_explanation / rule_build_counterfactual
// (ostis_kb_user_ecosystem/rules/rules_explanation.scs) -- один агент на
// все четыре правила, тем же приёмом, что и AgentComputePiMetric для
// π_proc/π_dec/π_data: конкретный тип объяснения выбирает вызывающая
// сторона аргументом, а не сам агент.
//
// Аргументы sc-действия action_build_explanation:
//   1) explanandum            -- объясняемый экземпляр concept_processing_result
//                                 (либо его подкласса, напр. concept_integrated_result);
//                                 обязателен
//   2) explanation_type       -- класс создаваемого объяснения
//                                 (concept_explanation_local/global/contrastive/
//                                 counterfactual/... -- concepts/explanations.scs);
//                                 обязателен
//   3) uses_results_group     -- необязательный узел-группа: элементы,
//                                 на которые исходящими ConstPermPosArc
//                                 указывает группа, становятся целями
//                                 роли rrel_uses_results
//   4) applies_rules_group    -- необязательная группа для rrel_applies_rules
//   5) based_on_group         -- необязательная группа для rrel_based_on
//   6) construction_time_link -- необязательная sc-ссылка с меткой времени
//                                 для rrel_construction_time (агент не
//                                 обращается к системным часам сам -- см.
//                                 аналогичное решение по rrel_timestamp
//                                 в AGENTS_AUDIT.md §7.3)
//
// Результат: создаётся новый узел объяснения (explanation), типизированный
// переданным explanation_type, и связывается с explanandum через
// nrel_supported_by_explanation (домен -- concept_processing_result,
// relations/relations_provenance.scs). Если explanandum уже классифицирован
// как concept_integrated_result -- дополнительно устанавливается
// nrel_explained_by (домен -- concept_integrated_result), как в
// scenarios/scenario_explanation_construction.scs. Одновременно порождается
// овеществлённый concept_explanation_construction_act со всеми пятью
// ролевыми связями шаблона prov_template_explanation (rrel_explanandum,
// rrel_explanation_type, rrel_uses_results, rrel_applies_rules,
// rrel_based_on, rrel_construction_time -- последние четыре только если
// соответствующий аргумент передан).
//
// Отсутствующий explanandum или explanation_type -- ошибка (FinishWithError).
// Отсутствующие группы/метка времени -- не ошибка, соответствующие роли
// просто не создаются (см. AgentComputePiMetric -- тот же принцип
// "нет данных = ничего не пишем", а не "ошибка").
//
// Не реализовано:
//  - содержательное текстовое основание объяснения (nrel_main_idtf узла
//    explanation с описанием, как в сценарии) -- агент строит только
//    типизированную и провенансно-связанную структуру, не генерирует
//    естественноязыковой текст;
//  - специфичный для типа контент: для contrastive -- вычисление различий
//    между X и Y, для counterfactual -- минимальное изменение входа;
//    агент не выводит эти данные сам, их обязан подготовить и передать
//    вызывающий код через uses_results_group/based_on_group;
//  - связывание использованных правил/источников через nrel_prov_used
//    (PROV:used, concepts/provenance.scs) -- его домен
//    concept_prov_activity -> concept_prov_entity не установлен для узлов
//    объяснения/правил/источников; типизировать их так в этом агенте
//    значило бы вводить непроверенную онтологию, а не переиспользовать
//    существующую (тот же принцип, что заблокировал AGENTS_AUDIT.md §7.4);
//  - агент не проверяет, что explanation_type действительно является
//    подклассом concept_explanation -- как и остальные агенты проекта, он
//    доверяет типизации, переданной вызывающей стороной.
class AgentBuildExplanation : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
