#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_assignment_admissibility (rules/rules_allocation.scs,
// agent_task_allocator) и шаг 2.3 жадной процедуры семантического
// распределения гл. 2:
//   assign(σ, s) = 1 ⟺ match(σ, s | C) ≥ θ ∧ valid(C, L_s) = 1
//                     ∧ permit(view(C, s), s, C) = 1 ∧ available(s, C) = 1;
//   S_{σ,C} = {s ∈ S | assign(σ, s) = 1};
//   χ(σ) ⊆ S_{σ,C} -- k(σ) участников с наибольшими match(σ, s | C).
//
// Объём согласован комиссией 13.09.2026, протокол и обоснование по
// каждому решению -- agents_vault/AgentAllocateSubtasks Commission.md.
//
// ЧТО ПРОВЕРЯЕТСЯ АГЕНТОМ (три конъюнкта из четырёх):
//  - available(s, C): у участника есть запись реестра компетенций
//    (nrel_entry_participant) со статусом concept_registry_status_active.
//    Основание -- affirmation_registry_required_for_assignment
//    (concepts/competency_registry.scs): "nrel_assigned_to(σ, s)
//    допустимо только для участника с активной записью".
//  - permit(view(C, s), s, C): политика доступа записи
//    (nrel_entry_policy) разрешает участника отношением nrel_permits
//    (relations/relations_control.scs).
//  - match(σ, s | C) ≥ θ: значение берётся из реифицированной оценки
//    concept_match_assessment (роли rrel_subtask/rrel_assignee, значение
//    nrel_match_value), порог θ -- аргумент действия.
//
// ЧТО НЕ ВЫЧИСЛЯЕТСЯ (принимается фактом от вызывающей стороны -- тот же
// приём, что у AgentHandleNoAdmissibleAssignment, AgentExcludeFromCoalition
// и AgentReassignOnResourceExhaustion):
//  - значение match: из шести слагаемых eq:semantic_match в гл. 2
//    формализованы только sim_T (нормированная мера Ву—Палмера,
//    eq:sim_type -- в KB пока отсутствует, отдельная задача
//    AgentComputeSimType) и trust (свёртка Σ λ_k t_k). Сложить
//    взвешенную сумму с четырьмя неопределёнными слагаемыми значило бы
//    изобрести формулу: получилось бы число, похожее на match, но им не
//    являющееся, и оно попало бы в акт назначения как факт;
//  - valid(C, L_s): предиката сравнения контекста с ресурсными
//    ограничениями L_s не задают ни гл. 2, ни KB (экземпляры L_s --
//    естественно-языковые ссылки вида "Требует GPU >= 6 ГБ VRAM").
//    Конъюнкт НЕ считается выполненным по умолчанию -- он не
//    проверяется, и это именованный открытый хвост. Отсутствие факта
//    не превращается в истину молча;
//  - k(σ): кратность назначения определяется типом маршрута, но ни один
//    concept_route_* в базе не привязан к запросу и носителя k(σ) нет --
//    передаётся аргументом;
//  - consistent(C, χ): связывающее ограничение между назначениями
//    разных подзадач. При непустом множестве таких условий задача
//    перестаёт быть сепарабельной, целевая функция не субмодулярна и
//    гарантии у жадной процедуры нет (гл. 2). Агент работает над одной
//    подзадачей, где ограничение выполнено тождественно и по
//    утверждению об оптимальности жадного назначения шаг 2.3 даёт
//    точное, а не приближённое решение.
//
// Аргументы sc-действия action_allocate_subtasks:
//   1) subtask        -- σ, распределяемая подзадача
//   2) thresholdLink  -- θ, порог применимости (link, [0,1]); аргумент,
//                        а не константа: θ объявлен в гл. 2 настраиваемым
//                        параметром, фиксируемым до оценочного прогона,
//                        и зашивание его в C++ лишило бы работу
//                        воспроизводимости
//   3) multiplicityLink -- k(σ), кратность назначения (link, целое ≥ 1)
//   4) context        -- C, контекст взаимодействия (роль rrel_context акта)
//   5) assignmentTime -- метка времени назначения (link) для
//                        rrel_assignment_time: prov_template_assignment
//                        требует эту роль, а источника времени в проекте
//                        нет ни у одного агента (AGENTS_AUDIT.md §7.3),
//                        поэтому она передаётся, а не синтезируется
//   6) resultLevel    -- узел, под которым размещаются индикатор и акты
//
// Результат: indicator = 1.0, если |S_{σ,C}| ≥ k(σ) и назначение
// построено; 0.0, если допустимых исполнителей меньше k(σ) -- это сигнал
// вызывающей стороне на путь AgentHandleNoAdmissibleAssignment (шаг 2.2
// листинга), сам агент ни повторной декомпозиции, ни отказа не порождает.
// Тот же приём "1.0 = проблем нет", что у остальных агентов модуля.
//
// При indicator = 1.0 для каждого выбранного s создаётся:
//  - nrel_assigned_to(σ, s) -- отношение не функционально (гл. 2:
//    подзадача может быть назначена нескольким участникам при k(σ) > 1);
//  - акт concept_process_allocation с ролями rrel_subtask, rrel_assignee,
//    rrel_context, rrel_assignment_time, rrel_required_confidence (= θ),
//    rrel_priority (перенесён с подзадачи по nrel_subtask_priority, не
//    придуман) и rrel_initiator (= agent_task_allocator). Сам акт и есть
//    фиксация в R_prov (шаг 2.4 листинга): по prov_template_assignment
//    провенансом акта назначения служит его реификация, а отдельная
//    запись concept_provenance_record сюда не навешивается, поскольку
//    домен nrel_has_provenance её не допускает.
//
// Не реализовано:
//  - повторная декомпозиция при |S_{σ,C}| < k(σ) (шаг 2.2а) --
//    rule_apply_decomposition_pattern не реализовано;
//  - переназначение по бюджету b_q (шаг 2.5) -- область
//    AgentReassignSubtask (AGENTS_AUDIT.md §7.5);
//  - rrel_priority не создаётся, если у подзадачи нет
//    nrel_subtask_priority (агент не подставляет значение по умолчанию);
//  - только IsElement на аргументах, как и у остальных агентов модуля.
class AgentAllocateSubtasks : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
