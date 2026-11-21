#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для первого агента rules/rules_allocation.scs
// (agent_task_allocator) -- rule_handle_no_admissible_assignment.
// concept_subtask_composite/concept_integrity_violation -- уже
// существующие элементы онтологии (concepts/tasks.scs,
// concepts/integrity.scs; concept_integrity_violation здесь
// продублирован, как и в SecurityKeynodes -- см. её же комментарий).
// nrel_concerns переиспользуется из IntegrityKeynodes (концептуально
// общий для всех агентов, создающих нарушения целостности), поэтому
// здесь не дублируется.
class AllocationKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const concept_subtask_composite{"concept_subtask_composite", ScType::NodeConstClass};
  static inline ScKeynode const concept_integrity_violation{"concept_integrity_violation", ScType::NodeConstClass};

  static inline ScKeynode const action_handle_no_admissible_assignment{
      "action_handle_no_admissible_assignment", ScType::NodeConstClass};

  // --- AgentAllocateSubtasks (rule_assignment_admissibility + шаг 2.3
  // жадной процедуры гл. 2). Все элементы ниже уже существуют в
  // онтологии; ScKeynode находит их, а не создаёт дубликаты.

  // Реифицированная оценка применимости пары (σ, s) и её значение
  // (concepts/capabilities.scs, relations/relations_execution.scs) --
  // носитель match(σ, s | C), вычисляемого вне агента.
  static inline ScKeynode const concept_match_assessment{"concept_match_assessment", ScType::NodeConstClass};
  static inline ScKeynode const nrel_match_value{"nrel_match_value", ScType::ConstNodeNonRole};

  // Реестр компетенций (concepts/competency_registry.scs) -- источник
  // available(s, C): назначение допустимо только участнику с активной
  // записью (affirmation_registry_required_for_assignment).
  static inline ScKeynode const nrel_entry_participant{"nrel_entry_participant", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_entry_status{"nrel_entry_status", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_entry_policy{"nrel_entry_policy", ScType::ConstNodeNonRole};
  static inline ScKeynode const concept_registry_status_active{
      "concept_registry_status_active", ScType::NodeConstClass};

  // permit(view(C, s), s, C): политика доступа разрешает участника
  // (relations/relations_control.scs).
  static inline ScKeynode const nrel_permits{"nrel_permits", ScType::ConstNodeNonRole};

  // Результат назначения: отношение подзадача -> исполнитель
  // (relations/relations_task.scs) и реифицированный акт назначения
  // с шестью ролями (relations/role_relations.scs,
  // concepts/lifecycle_processes.scs).
  static inline ScKeynode const nrel_assigned_to{"nrel_assigned_to", ScType::ConstNodeNonRole};
  static inline ScKeynode const concept_process_allocation{"concept_process_allocation", ScType::NodeConstClass};
  static inline ScKeynode const rrel_subtask{"rrel_subtask", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_assignee{"rrel_assignee", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_context{"rrel_context", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_assignment_time{"rrel_assignment_time", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_required_confidence{"rrel_required_confidence", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_priority{"rrel_priority", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_initiator{"rrel_initiator", ScType::ConstNodeRole};

  // Приоритет подзадачи (relations/relations_task.scs) -- переносится
  // на акт назначения в роли rrel_priority, а не придумывается:
  // rules_integrity_validation.scs требует у каждой подзадачи minCount 1
  // по nrel_subtask_priority, и concepts/tasks.scs прямо называет
  // уровень приоритета общим для подзадачи и акта назначения.
  static inline ScKeynode const nrel_subtask_priority{"nrel_subtask_priority", ScType::ConstNodeNonRole};

  // Инициатор акта назначения (concepts/sc_agents.scs) -- существующий
  // элемент онтологии с nrel_implements_process:
  // concept_process_allocation, то есть ровно тот, кому этот акт
  // принадлежит по модели. Отдельная запись concept_provenance_record
  // НЕ создаётся: акт назначения не входит в объявленный домен
  // nrel_has_provenance (relations/relations_provenance.scs), а
  // prov_template_assignment (axioms/axioms_provenance.scs)
  // прямо говорит, что провенансом акта назначения служит сама его
  // реификация. Навешивать сюда nrel_has_provenance значило бы ломать
  // уже зафиксированный домен -- ровно то, от чего отказался §7.4
  // аудита.
  static inline ScKeynode const agent_task_allocator{"agent_task_allocator", ScType::NodeConstClass};

  static inline ScKeynode const action_allocate_subtasks{"action_allocate_subtasks", ScType::NodeConstClass};
};
