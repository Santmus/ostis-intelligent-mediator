#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_right_to_explanation (rules/rules_security_privacy.scs):
// IF delivered_to(ir, u) ∧ request_type(q) ∈ {decision_support,
// recommendation} THEN explained_by(ir, e) обязателен. Иначе -- нарушение
// concept_property_right_to_explanation, ir не может быть выдан
// пользователю.
//
// Аргументы sc-действия action_validate_right_to_explanation:
//   1) integrated_result -- ir, проверяемый интегрированный результат /
//                            коллективное решение (агент доверяет
//                            вызывающей стороне относительно класса, как
//                            и везде в модуле)
//   2) request           -- q, запрос, породивший ir; классифицируется
//                            ли он concept_request_decision_support/
//                            concept_request_recommendation -- вторая
//                            половина условия гейта. Формального
//                            однохопового пути ir -> q в онтологии нет
//                            (ir не хранит явную ссылку на исходный
//                            запрос), поэтому запрос передаётся отдельным
//                            аргументом, а не выводится агентом
//                            самостоятельно -- тот же приём, что у
//                            AgentExcludeFromCoalition (community
//                            передаётся, а не выводится).
//   3) result_level       -- узел, под которым размещаются индикатор и,
//                            при обнаруженном нарушении, узел нарушения
//
// Гейт (обе части обязательны, иначе проверка вырожденно проходит):
//   - ∃ хотя бы одна дуга ir -[delivered_to]-> · (наличие КОНКРЕТНОГО
//     получателя u не проверяется -- в консеквенте правила u не
//     упоминается);
//   - request классифицирован concept_request_decision_support ИЛИ
//     concept_request_recommendation.
// Если гейт не сработал -- valid = true независимо от explained_by
// (правило не предъявляет требования вне этих двух условий).
//
// Нарушение: узел классифицируется ОБОИМИ классами сразу --
// concept_integrity_violation (общая иерархия нарушений проекта) и
// concept_property_right_to_explanation (уже существующий класс
// concepts/security_privacy.scs) -- по буквальному образцу соседнего
// правила того же файла, rule_enforce_consent_check ("создать
// concept_integrity_violation типа concept_property_consent_compliance").
// Новый подкласс, в отличие от AgentCheckAcyclicity/
// AgentValidateCardinalityConstraint, здесь не потребовался.
//
// Наблюдение по терминологии (не исправляется здесь): текст ЭТОГО
// правила требует explained_by(ir,e) (relations/relations_provenance.scs,
// nrel_explained_by: concept_integrated_result -> concept_explanation --
// та же связь, что проверяет AgentValidateIntegration и создаёт
// AgentBuildExplanation), тогда как соседняя АКСИОМА
// axiom_decision_requires_explanation (axioms/axioms_security.scs)
// требует supported_by_explanation(d,e) (nrel_supported_by_explanation:
// concept_processing_result -> concept_explanation) -- другое отношение
// для похожего по смыслу требования. Реализовано буквально по тексту
// ПРАВИЛА (nrel_explained_by), поскольку операционализируется именно
// rule_right_to_explanation; согласование терминологии правила и
// аксиомы этого агента не касается.
//
// Не реализовано:
//  - "agent_explanation_builder обязан построить" -- агент не вызывает
//    AgentBuildExplanation при обнаруженном нарушении, только
//    сигнализирует его отсутствие (та же граница ответственности, что
//    у AgentValidateIntegration/AgentValidateProvenanceChain: они
//    проверяют состояние, а не чинят его);
//  - "ir не может быть выдан пользователю" -- нет реализованного
//    AgentPublishResult (AGENTS_AUDIT.md §7.5), который обязан был бы
//    прочитать этот индикатор перед публикацией;
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentValidateRightToExplanation : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
