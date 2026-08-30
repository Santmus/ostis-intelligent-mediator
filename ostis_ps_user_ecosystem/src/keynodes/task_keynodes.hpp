#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для трёх агентов agent_cooperative_executor
// (rule_dispatch_ready_subtask, rule_register_partial_result,
// rule_propagate_to_dependent -- rules/rules_cooperation.scs). Все
// перечисленные ниже -- уже существующие элементы онтологии
// (relations/relations_execution.scs, relations/relations_task.scs,
// relations/relations_context.scs, relations/role_relations.scs,
// concepts/tasks.scs, concepts/interactions.scs,
// concepts/knowledge_exchange.scs, concepts/sc_agents.scs); системные
// идентификаторы совпадают, поэтому ScKeynode находит существующие
// узлы, а не создаёт дубликаты. action_dispatch_ready_subtask,
// action_register_partial_result и action_propagate_to_dependent также
// объявлены в SC-коде для документации и единообразия модели.
class TaskKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_assigned_to{"nrel_assigned_to", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_subtask_status{"nrel_subtask_status", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_subtask_depends_on{"nrel_subtask_depends_on", ScType::ConstNodeNonRole};

  // Провенансный триплет rule_register_partial_result -- не
  // функциональные отношения (у результата/подзадачи может быть
  // несколько дуг каждого).
  static inline ScKeynode const nrel_produces{"nrel_produces", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_caused_by_subtask{"nrel_caused_by_subtask", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_generated_in_context{"nrel_generated_in_context", ScType::ConstNodeNonRole};

  // Результат сделан входом зависимой подзадачи (relations_execution.scs,
  // AGENTS_AUDIT.md п.6.10) -- подзадача как таковая не является
  // участником коммуникации.
  static inline ScKeynode const nrel_is_input_for{"nrel_is_input_for", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_subtask_status_proposed{
      "concept_subtask_status_proposed", ScType::NodeConstClass};
  static inline ScKeynode const concept_subtask_status_in_progress{
      "concept_subtask_status_in_progress", ScType::NodeConstClass};
  static inline ScKeynode const concept_subtask_status_completed{
      "concept_subtask_status_completed", ScType::NodeConstClass};

  static inline ScKeynode const concept_partial_result{"concept_partial_result", ScType::NodeConstClass};
  static inline ScKeynode const concept_processing_result{"concept_processing_result", ScType::NodeConstClass};

  // Акт исполнения и запись аудита (concepts/execution_acts.scs) --
  // овеществление факта регистрации частичного результата, требуемое
  // финальной моделью (AGENTS_AUDIT.md, п.6.2): результат не считается
  // зарегистрированным без обоих.
  static inline ScKeynode const concept_execution_act{"concept_execution_act", ScType::NodeConstClass};
  static inline ScKeynode const concept_execution_act_successful{
      "concept_execution_act_successful", ScType::NodeConstClass};
  static inline ScKeynode const nrel_executes{"nrel_executes", ScType::ConstNodeNonRole};
  static inline ScKeynode const concept_audit_record{"concept_audit_record", ScType::NodeConstClass};
  static inline ScKeynode const concept_audit_record_execution{
      "concept_audit_record_execution", ScType::NodeConstClass};
  static inline ScKeynode const nrel_recorded_in{"nrel_recorded_in", ScType::ConstNodeNonRole};
  static inline ScKeynode const concept_provenance_record{"concept_provenance_record", ScType::NodeConstClass};
  static inline ScKeynode const nrel_has_provenance{"nrel_has_provenance", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_generated_by{"nrel_generated_by", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_recorded_by_sc_agent{"nrel_recorded_by_sc_agent", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_knowledge_exchange_act{
      "concept_knowledge_exchange_act", ScType::NodeConstClass};
  static inline ScKeynode const concept_act_request{"concept_act_request", ScType::NodeConstClass};
  static inline ScKeynode const concept_act_inform{"concept_act_inform", ScType::NodeConstClass};

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
  static inline ScKeynode const action_register_partial_result{
      "action_register_partial_result", ScType::NodeConstClass};
  static inline ScKeynode const action_propagate_to_dependent{
      "action_propagate_to_dependent", ScType::NodeConstClass};
};
