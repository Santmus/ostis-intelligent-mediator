#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_check_acyclicity (rules/rules_decomposition.scs,
// agent_task_decomposer): после декомпозиции запроса q проверить
// acyclic(depends_on) для множества подзадач
// T(q) = {σ : decomposed_into(q, σ)} (concepts/tasks.scs,
// definition_dependency_graph) над отношением nrel_subtask_depends_on.
// При обнаружении цикла создаётся concept_acyclicity_constraint_violation
// (concepts/integrity.scs -- подкласс concept_integrity_violation,
// формализован вместе с этим агентом по тому же образцу, что
// concept_cardinality_constraint_violation/concept_existence_constraint_violation
// у AgentValidateCardinalityConstraint).
//
// Аргументы sc-действия action_check_acyclicity:
//   1) request      -- экземпляр concept_user_request, декомпозиция
//                       которого проверяется (агент доверяет вызывающей
//                       стороне относительно класса, как и везде в
//                       модуле)
//   2) result_level  -- узел, под которым размещаются индикатор и, при
//                       обнаружении цикла, нарушение
//
// Обход: DFS с различением visited (все когда-либо посещённые узлы) и
// onStack (узлы на текущем пути обхода) -- цикл обнаруживается только
// при переходе в узел из onStack; повторное посещение узла, уже
// покинувшего стек (сходящиеся, но не циклические пути DAG -- например,
// две подзадачи, зависящие от одной общей), не является нарушением.
//
// Результат: valid = ¬∃ цикл в подграфе nrel_subtask_depends_on,
// достижимом из T(request); записывается как 1.0/0.0 в value_link узла
// result_level (тот же приём, что у AgentValidateProvenanceChain). При
// invalid дополнительно создаётся concept_acyclicity_constraint_violation,
// присоединяется под result_level и связывается с request через
// nrel_concerns.
//
// Не реализовано:
//  - "инициировать пересмотр шаблона декомпозиции" -- agent_task_decomposer
//    сам не реализован (rule_apply_decomposition_pattern/
//    rule_recursive_decomposition -- PARTIAL по Implementation Backlog.md,
//    требуют sim_T и библиотеку шаблонов, не определённых в тексте);
//  - обход проверяет ацикличность подграфа, ДОСТИЖИМОГО из T(request)
//    по nrel_subtask_depends_on, а не только рёбер строго между
//    элементами T(request) -- по всем сценариям проекта дуги
//    depends_on существуют только внутри одной декомпозиции, так что
//    разницы на практике нет;
//  - только IsElement на аргументах, как и у остальных агентов модуля
//    (класс request как concept_user_request не проверяется).
class AgentCheckAcyclicity : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
