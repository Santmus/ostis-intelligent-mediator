#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует правило rule_dispatch_ready_subtask
// (ostis_kb_user_ecosystem/rules/rules_cooperation.scs):
//   IF assigned_to(sigma, s) AND subtask_status(sigma, proposed)
//      AND forall sigma'. depends_on(sigma, sigma')
//                          -> subtask_status(sigma', completed)
//   THEN subtask_status(sigma) := in_progress;
//        отправить sigma системе s через concept_act_request
//
// В отличие от предыдущих агентов, здесь не нужно передавать
// исполнителя s, статусы зависимостей и т.д. отдельными аргументами --
// всё это уже связано с самой подзадачей через существующие отношения
// онтологии (nrel_assigned_to, nrel_subtask_status,
// nrel_subtask_depends_on) и вычитывается по одному аргументу.
//
// Аргументы sc-действия action_dispatch_ready_subtask:
//   1) subtask      -- существующий экземпляр concept_subtask
//   2) result_level -- узел для индикатора (1 -- подзадача
//                      диспетчеризована, 0 -- условие не выполнено);
//                      если у него ещё нет числовой sc-ссылки, она
//                      создаётся
//
// "Не готова к запуску" (нет исполнителя, статус не proposed, есть
// незавершённая зависимость) -- это НЕ ошибка действия, а обычный
// результат с индикатором 0: агент рассчитан на то, что его можно
// безопасно вызывать спекулятивно (например, оркестратором на каждой
// proposed-подзадаче периодически).
//
// Подзадача без зависимостей (нет исходящих nrel_subtask_depends_on)
// считается готовой по зависимостям автоматически -- пустое
// "для всех" истинно вакуумно, как и предписывает формула правила.
//
// При успешной диспетчеризации:
//   - существующая дуга nrel_subtask_status стирается и создаётся
//     новая, на concept_subtask_status_in_progress (nrel_subtask_status
//     -- функциональное отношение, у подзадачи не может быть двух
//     статусов одновременно);
//   - генерируется экземпляр concept_knowledge_exchange_act с ролями
//     rrel_sender=agent_cooperative_executor, rrel_receiver=s,
//     rrel_act_type=concept_act_request, rrel_payload=subtask.
//     rrel_context и rrel_timestamp не заполняются -- правило не
//     указывает их источник, а агенту неоткуда взять текущее время
//     или контекст взаимодействия.
class AgentDispatchReadySubtask : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
