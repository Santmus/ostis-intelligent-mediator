#pragma once

#include <sc-memory/sc_keynodes.hpp>

// Ключевые sc-элементы для двух реактивных агентов реконфигурации
// коалиции (rules/rules_coalition_reconfiguration.scs): исключение
// отказавшей системы (rule_on_system_failure) и массовое переназначение
// подзадач при истощении ресурса (rule_on_resource_exhaustion). Все
// перечисленные ниже -- уже существующие элементы онтологии
// (relations/relations_structural.scs, concepts/tasks.scs,
// concepts/communities.scs); системные идентификаторы совпадают, поэтому
// ScKeynode находит существующие узлы. action_exclude_from_coalition и
// action_reassign_on_resource_exhaustion также объявлены в SC-коде
// (rules/rules_coalition_reconfiguration.scs) для документации и
// единообразия модели. nrel_assigned_to/nrel_subtask_status уже
// объявлены в TaskKeynodes -- продублированы здесь намеренно (тот же
// идентификатор резолвится в тот же узел), чтобы этот файл не тянул
// зависимость от keynodes другого агентного семейства.
class CoalitionKeynodes : public ScKeynodes
{
public:
  static inline ScKeynode const nrel_assigned_to{"nrel_assigned_to", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_subtask_status{"nrel_subtask_status", ScType::ConstNodeNonRole};
  static inline ScKeynode const nrel_member_of{"nrel_member_of", ScType::ConstNodeNonRole};

  static inline ScKeynode const concept_subtask_status_failed{
      "concept_subtask_status_failed", ScType::NodeConstClass};
  static inline ScKeynode const concept_subtask_status_reassigned{
      "concept_subtask_status_reassigned", ScType::NodeConstClass};

  static inline ScKeynode const action_exclude_from_coalition{
      "action_exclude_from_coalition", ScType::NodeConstClass};
  static inline ScKeynode const action_reassign_on_resource_exhaustion{
      "action_reassign_on_resource_exhaustion", ScType::NodeConstClass};
};
