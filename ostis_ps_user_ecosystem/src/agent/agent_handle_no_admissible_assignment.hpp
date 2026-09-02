#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует механическую часть rule_handle_no_admissible_assignment
// (rules/rules_allocation.scs, agent_task_allocator):
//   If: ∀s ∈ S. ¬(match(σ, s) ≥ θ ∧ valid(C, L_s)),
//   Then: запустить rule_recursive_decomposition для σ либо
//         инициировать concept_integrity_violation и вернуть
//         управление agent_task_decomposer.
//
// Посылка ("нет допустимого исполнителя") НЕ проверяется этим агентом:
// match(σ,s) определена лишь частично -- rule_compute_match_function
// называет 6 слагаемых (sim_T, sim_I, sim_O, ctx_compat, qos_score,
// trust), но только trust(s,τ,h) уже вычислим (AgentAggregateTrust);
// остальные пять не формализованы нигде в тексте (Implementation
// Backlog.md). Агент принимает посылку уже установленной вызывающей
// стороной -- тот же приём, что у AgentExcludeFromCoalition/
// AgentReassignOnResourceExhaustion в отношении failed/resource_exhausted.
//
// Ветвление консеквента ("decomposition либо violation") разрешается
// через affirmation_subtask_executability (concepts/tasks.scs: всякая
// подзадача либо исполнима, либо декомпозируема) -- если σ
// классифицирована concept_subtask_composite, путь декомпозиции
// применим; иначе (атомарная подзадача без допустимого исполнителя) --
// нарушение целостности.
//
// Аргументы sc-действия action_handle_no_admissible_assignment:
//   1) subtask      -- σ, подзадача без допустимого исполнителя (агент
//                       доверяет вызывающей стороне относительно
//                       посылки, как описано выше)
//   2) result_level  -- узел, под которым размещаются индикатор и, при
//                       атомарной подзадаче, нарушение
//
// Результат: indicator = 1.0, если σ композитна (путь декомпозиции
// применим -- сама декомпозиция не выполняется, см. Limitations); 0.0,
// если σ атомарна (создано нарушение). Тот же приём "1.0 = проблем
// нет", что у AgentValidateCardinalityConstraint и других.
//
// Не реализовано:
//  - "запустить rule_recursive_decomposition" -- rule_apply_decomposition_pattern
//    само не реализовано (требует sim_T и библиотеку шаблонов, не
//    определённых в тексте, см. Implementation Backlog.md); для
//    композитной подзадачи агент лишь распознаёт применимость этого
//    пути, саму декомпозицию не выполняет;
//  - "вернуть управление agent_task_decomposer" -- нет реализованного
//    оркестратора жизненного цикла (AGENTS_AUDIT.md §7.1), как и везде
//    в проекте;
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentHandleNoAdmissibleAssignment : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
