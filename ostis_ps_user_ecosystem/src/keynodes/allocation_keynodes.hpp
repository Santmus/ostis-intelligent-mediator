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
};
