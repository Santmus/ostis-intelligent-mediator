#include "agent_allocate_subtasks.hpp"

#include <algorithm>
#include <vector>

#include <sc-memory/sc_memory_headers.hpp>

#include "common/float_link_io.hpp"
#include "common/relation_io.hpp"
#include "keynodes/allocation_keynodes.hpp"

using trust_utils::IsUnitInterval;
using trust_utils::LinkByRelation;
using trust_utils::LinkByRole;
using trust_utils::ReadFloatLinkContent;
using trust_utils::TargetOf;
using trust_utils::WriteFloatLinkContent;

namespace
{
// Кандидат на исполнение подзадачи: участник и значение match(σ, s | C),
// прочитанное из реифицированной оценки, а не вычисленное здесь.
struct Candidate
{
  ScAddr system;
  float match;
};

// Первая цель ролевого отношения roleAddr у sourceAddr. В отличие от
// ForEachRoleTarget применяется к ролям, у которых по модели ровно одна
// цель (rrel_subtask, rrel_assignee у одной оценки применимости).
ScAddr RoleTargetOf(ScAgentContext & context, ScAddr const & sourceAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      sourceAddr, ScType::ConstPermPosArc, ScType::Unknown, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Запись реестра компетенций, относящаяся к участнику systemAddr
// (nrel_entry_participant -- функциональное отношение записи на
// участника, поэтому поиск ведётся от участника обратно по дуге).
ScAddr RegistryEntryOf(ScAgentContext & context, ScAddr const & systemAddr)
{
  ScIterator5Ptr const it5 = context.CreateIterator5(
      ScType::Unknown,
      ScType::ConstCommonArc,
      systemAddr,
      ScType::ConstPermPosArc,
      AllocationKeynodes::nrel_entry_participant);
  if (it5->Next())
    return it5->Get(0);
  return ScAddr::Empty;
}

// available(s, C): запись реестра существует и её статус активен.
// Основание -- affirmation_registry_required_for_assignment
// (concepts/competency_registry.scs). Отсутствие записи означает, что
// условие НЕ установлено, а не что оно выполнено.
bool IsAvailable(ScAgentContext & context, ScAddr const & entryAddr)
{
  if (!context.IsElement(entryAddr))
    return false;

  return TargetOf(context, entryAddr, AllocationKeynodes::nrel_entry_status)
         == AllocationKeynodes::concept_registry_status_active;
}

// permit(view(C, s), s, C): хотя бы одна политика доступа, приписанная
// записи реестра (nrel_entry_policy), разрешает участника отношением
// nrel_permits. Запись без политики разрешением не считается: отсутствие
// факта разрешения -- это отсутствие факта, а не разрешение по умолчанию.
bool IsPermitted(ScAgentContext & context, ScAddr const & entryAddr, ScAddr const & systemAddr)
{
  if (!context.IsElement(entryAddr))
    return false;

  ScIterator5Ptr const policiesIt = context.CreateIterator5(
      entryAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      AllocationKeynodes::nrel_entry_policy);
  while (policiesIt->Next())
  {
    ScIterator5Ptr const permitsIt = context.CreateIterator5(
        policiesIt->Get(2),
        ScType::ConstCommonArc,
        systemAddr,
        ScType::ConstPermPosArc,
        AllocationKeynodes::nrel_permits);
    if (permitsIt->Next())
      return true;
  }
  return false;
}

// Строит S_{σ,C}: перебирает реифицированные оценки применимости,
// относящиеся к σ, и оставляет тех кандидатов, для которых выполнены
// три проверяемых конъюнкта допустимости. valid(C, L_s) не проверяется
// (см. заголовок агента).
std::vector<Candidate> CollectAdmissible(ScAgentContext & context, ScAddr const & subtaskAddr, float threshold)
{
  std::vector<Candidate> admissible;

  ScIterator3Ptr const assessmentsIt = context.CreateIterator3(
      AllocationKeynodes::concept_match_assessment, ScType::ConstPermPosArc, ScType::Unknown);
  while (assessmentsIt->Next())
  {
    ScAddr const assessmentAddr = assessmentsIt->Get(2);
    if (RoleTargetOf(context, assessmentAddr, AllocationKeynodes::rrel_subtask) != subtaskAddr)
      continue;

    ScAddr const systemAddr = RoleTargetOf(context, assessmentAddr, AllocationKeynodes::rrel_assignee);
    if (!context.IsElement(systemAddr))
      continue;

    ScAddr const valueLinkAddr = TargetOf(context, assessmentAddr, AllocationKeynodes::nrel_match_value);
    float matchValue = 0.0f;
    if (!context.IsElement(valueLinkAddr) || !ReadFloatLinkContent(context, valueLinkAddr, matchValue))
      continue;

    if (matchValue < threshold)
      continue;

    ScAddr const entryAddr = RegistryEntryOf(context, systemAddr);
    if (!IsAvailable(context, entryAddr) || !IsPermitted(context, entryAddr, systemAddr))
      continue;

    admissible.push_back({systemAddr, matchValue});
  }

  return admissible;
}
}  // namespace

ScAddr AgentAllocateSubtasks::GetActionClass() const
{
  return AllocationKeynodes::action_allocate_subtasks;
}

ScResult AgentAllocateSubtasks::DoProgram(ScAction & action)
{
  auto const & [subtaskAddr, thresholdLinkAddr, multiplicityLinkAddr, contextAddr, assignmentTimeAddr, resultLevelAddr] =
      action.GetArguments<6>();
  if (!m_context.IsElement(subtaskAddr) || !m_context.IsElement(thresholdLinkAddr)
      || !m_context.IsElement(multiplicityLinkAddr) || !m_context.IsElement(contextAddr)
      || !m_context.IsElement(assignmentTimeAddr) || !m_context.IsElement(resultLevelAddr))
  {
    m_logger.Error("AgentAllocateSubtasks: action does not have all six arguments.");
    return action.FinishWithError();
  }

  float threshold = 0.0f;
  if (!ReadFloatLinkContent(m_context, thresholdLinkAddr, threshold) || !IsUnitInterval(threshold))
  {
    m_logger.Error("AgentAllocateSubtasks: threshold is not a number within [0, 1].");
    return action.FinishWithError();
  }

  float multiplicityValue = 0.0f;
  if (!ReadFloatLinkContent(m_context, multiplicityLinkAddr, multiplicityValue) || multiplicityValue < 1.0f)
  {
    m_logger.Error("AgentAllocateSubtasks: multiplicity k is not a number not less than 1.");
    return action.FinishWithError();
  }
  auto const multiplicity = static_cast<size_t>(multiplicityValue);

  std::vector<Candidate> admissible = CollectAdmissible(m_context, subtaskAddr, threshold);

  // Шаг 2.3 листинга: k(σ) участников с наибольшими match. stable_sort,
  // чтобы при равных значениях порядок определялся перебором базы, а не
  // произволом алгоритма сортировки.
  std::stable_sort(
      admissible.begin(),
      admissible.end(),
      [](Candidate const & left, Candidate const & right)
      {
        return left.match > right.match;
      });

  bool const allocated = admissible.size() >= multiplicity;

  if (allocated)
  {
    ScAddr const priorityAddr = TargetOf(m_context, subtaskAddr, AllocationKeynodes::nrel_subtask_priority);

    for (size_t i = 0; i < multiplicity; ++i)
    {
      ScAddr const systemAddr = admissible[i].system;

      LinkByRelation(m_context, subtaskAddr, systemAddr, AllocationKeynodes::nrel_assigned_to);

      ScAddr const actAddr = m_context.GenerateNode(ScType::ConstNode);
      m_context.GenerateConnector(
          ScType::ConstPermPosArc, AllocationKeynodes::concept_process_allocation, actAddr);
      m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, actAddr);

      LinkByRole(m_context, actAddr, subtaskAddr, AllocationKeynodes::rrel_subtask);
      LinkByRole(m_context, actAddr, systemAddr, AllocationKeynodes::rrel_assignee);
      LinkByRole(m_context, actAddr, contextAddr, AllocationKeynodes::rrel_context);
      LinkByRole(m_context, actAddr, assignmentTimeAddr, AllocationKeynodes::rrel_assignment_time);
      LinkByRole(m_context, actAddr, thresholdLinkAddr, AllocationKeynodes::rrel_required_confidence);
      LinkByRole(m_context, actAddr, AllocationKeynodes::agent_task_allocator, AllocationKeynodes::rrel_initiator);

      // Приоритет переносится с подзадачи; при его отсутствии роль не
      // создаётся -- значение по умолчанию не подставляется.
      if (m_context.IsElement(priorityAddr))
        LinkByRole(m_context, actAddr, priorityAddr, AllocationKeynodes::rrel_priority);
    }
  }

  ScAddr valueLinkAddr;
  ScIterator3Ptr const it3 =
      m_context.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  if (it3->Next())
    valueLinkAddr = it3->Get(2);
  else
  {
    valueLinkAddr = m_context.GenerateLink(ScType::ConstNodeLink);
    m_context.GenerateConnector(ScType::ConstPermPosArc, resultLevelAddr, valueLinkAddr);
  }
  WriteFloatLinkContent(m_context, valueLinkAddr, allocated ? 1.0f : 0.0f);

  action.FormResult(subtaskAddr, valueLinkAddr);
  m_logger.Debug(
      "AgentAllocateSubtasks: admissible = ", admissible.size(), ", k = ", multiplicity, ", allocated = ", allocated);

  return action.FinishSuccessfully();
}
