#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для агента диспетчеризации готовых подзадач
// (rule_dispatch_ready_subtask, rules/rules_cooperation.scs). Все
// перечисленные ниже -- уже существующие элементы онтологии
// (relations/relations_execution.scs, relations/relations_task.scs,
// relations/role_relations.scs, concepts/tasks.scs,
// concepts/knowledge_exchange.scs, concepts/sc_agents.scs); системные
// идентификаторы совпадают, поэтому ScKeynode находит существующие
// узлы, а не создаёт дубликаты. action_dispatch_ready_subtask также
// объявлен в SC-коде для документации и единообразия модели.
class TaskKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_assigned_to{"nrel_assigned_to", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_subtask_status{"nrel_subtask_status", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_subtask_depends_on{"nrel_subtask_depends_on", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_subtask_status_proposed{
      "concept_subtask_status_proposed", ScType::NodeConstClass};
  static inline ScKeynode const concept_subtask_status_in_progress{
      "concept_subtask_status_in_progress", ScType::NodeConstClass};
  static inline ScKeynode const concept_subtask_status_completed{
      "concept_subtask_status_completed", ScType::NodeConstClass};

  static inline ScKeynode const concept_knowledge_exchange_act{
      "concept_knowledge_exchange_act", ScType::NodeConstClass};
  static inline ScKeynode const concept_act_request{"concept_act_request", ScType::NodeConstClass};

  static inline ScKeynode const rrel_sender{"rrel_sender", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_receiver{"rrel_receiver", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_act_type{"rrel_act_type", ScType::ConstNodeRole};
  static inline ScKeynode const rrel_payload{"rrel_payload", ScType::ConstNodeRole};

  // Отправитель коммуникативного акта запроса: концептуальный
  // "хозяин" правил кооперации в каталоге concepts/sc_agents.scs.
  // Единственный реализованный агент в проекте не представлен
  // отдельным sc-узлом-идентичностью, поэтому используется уже
  // существующий формальный элемент онтологии, а не выдуманный.
  static inline ScKeynode const agent_cooperative_executor{"agent_cooperative_executor", ScType::NodeConstClass};

  static inline ScKeynode const action_dispatch_ready_subtask{
      "action_dispatch_ready_subtask", ScType::NodeConstClass};
};
