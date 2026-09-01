#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_on_resource_exhaustion (rules/rules_coalition_reconfiguration.scs):
//   При обнаружении нарушения L_s системы s (resource_exhausted)
//   установить subtask_status(σ, reassigned) для всех подзадач,
//   назначенных s; инициировать rule_coalition_optimization.
// Как и AgentExcludeFromCoalition, закрывает только детерминированную
// часть; rule_coalition_optimization не реализована и не инициируется
// отсюда -- та же причина (комбинаторная задача без алгоритма в тексте
// правил).
//
// Аргументы sc-действия action_reassign_on_resource_exhaustion:
//   1) system       -- система s, чьё ограничение ресурса L_s нарушено
//                       (сам факт нарушения -- внешний вход; агент не
//                       проверяет и не вычисляет resource_exhausted сам,
//                       он ему не передаётся ни в каком виде -- см.
//                       Limitations)
//   2) result_level -- узел, под которым размещается индикатор
//                       (количество переназначенных подзадач)
//
// "Все подзадачи, назначенные s" находятся обратным запросом по
// nrel_assigned_to (подзадача -> система) -- агент сам перечисляет их,
// вызывающей стороне не нужно передавать группу.
//
// Результат: у каждой найденной подзадачи статус переводится в
// concept_subtask_status_reassigned (через SetFunctionalRelationTarget --
// функциональное отношение, старое значение стирается); количество
// переназначенных подзадач записывается как число в value_link узла
// result_level. Ноль найденных подзадач -- 0.0, не ошибка (тот же
// принцип "нет данных = 0", что и у AgentComputePiMetric).
//
// Не реализовано:
//  - агент не проверяет и не вычисляет сам факт nарушения L_s
//    (resource_exhausted) -- принимает system как заданный вход и
//    действует безусловно; определение самого условия истощения ресурса
//    нигде в тексте правила не формализовано численно;
//  - запуск rule_coalition_optimization -- см. выше;
//  - переназначенные подзадачи не получают нового исполнителя -- статус
//    reassigned лишь помечает их как требующие нового назначения
//    (agent_task_allocator, не реализован);
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentReassignOnResourceExhaustion : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
