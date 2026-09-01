#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_on_system_failure (rules/rules_coalition_reconfiguration.scs):
//   If: subtask_status(σ,failed) ∧ assigned_to(σ,s) ∧ member_of(s,K),
//   Then: исключить s из коалиции K; запустить rule_coalition_optimization
//   для подзадач, ранее назначенных s, с обновлённым S.
// Этот агент закрывает только детерминированный state-transition ("исключить
// s из K"); rule_coalition_optimization -- комбинаторная задача о
// распределении без заданного алгоритма (rules/rules_allocation.scs,
// PARTIAL по Implementation Backlog.md) -- не реализована и не
// инициируется отсюда.
//
// Аргументы sc-действия action_exclude_from_coalition:
//   1) subtask   -- проверяемая подзадача σ
//   2) community -- проверяемая коалиция K (конкретный экземпляр
//                   concept_community_of_intelligent_systems, не
//                   выводится агентом сам -- см. Limitations)
//   3) result_level -- узел, под которым размещается индикатор
//
// Все три условия правила проверяются буквально, а не предполагаются:
// статус подзадачи должен быть именно concept_subtask_status_failed,
// исполнитель должен существовать (nrel_assigned_to), и между этим
// исполнителем и переданной community должна существовать конкретная
// дуга nrel_member_of -- ищется прямым запросом с обоими концами
// зафиксированными (subtask/community заданы аргументами), а не
// перебором всех коалиций исполнителя.
//
// Результат: если все три условия выполнены -- дуга
// исполнитель -[nrel_member_of]-> community стирается, индикатор = 1.0;
// иначе -- ничего не меняется, индикатор = 0.0 (невыполненное условие,
// не ошибка, тот же принцип, что и у AgentDispatchReadySubtask).
//
// Не реализовано:
//  - запуск rule_coalition_optimization -- см. выше;
//  - агент не принимает community "по умолчанию" (например, единственную
//    community исполнителя) -- вызывающая сторона обязана знать, из
//    какой коалиции исключать, и передать её явно; при членстве
//    исполнителя в нескольких community одновременно это исключает
//    ошибку "исключили не из той";
//  - только IsElement на subtask/community/result_level, как и у
//    остальных агентов модуля.
class AgentExcludeFromCoalition : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
