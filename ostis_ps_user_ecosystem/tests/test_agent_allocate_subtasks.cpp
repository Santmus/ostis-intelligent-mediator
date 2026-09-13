#include <algorithm>
#include <string>
#include <vector>

#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_allocate_subtasks.hpp"
#include "keynodes/allocation_keynodes.hpp"

using AgentAllocateSubtasksTest = ScMemoryTest;

namespace
{
ScAddr MakeLink(ScAgentContext & ctx, std::string const & content)
{
  ScAddr const linkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(linkAddr, content);
  return linkAddr;
}

void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & dst, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, dst);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

void LinkByRole(ScAgentContext & ctx, ScAddr const & src, ScAddr const & dst, ScAddr const & roleAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstPermPosArc, src, dst);
  ctx.GenerateConnector(ScType::ConstPermPosArc, roleAddr, arcAddr);
}

ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

ScAddr RoleTargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstPermPosArc, ScType::Unknown, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

float ReadIndicator(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
}

// Все участники, которым назначена подзадача (nrel_assigned_to не
// функционально: при k(σ) > 1 назначений несколько).
std::vector<ScAddr> AssigneesOf(ScAgentContext & ctx, ScAddr const & subtaskAddr)
{
  std::vector<ScAddr> assignees;
  ScIterator5Ptr const it5 = ctx.CreateIterator5(
      subtaskAddr,
      ScType::ConstCommonArc,
      ScType::Unknown,
      ScType::ConstPermPosArc,
      AllocationKeynodes::nrel_assigned_to);
  while (it5->Next())
    assignees.push_back(it5->Get(2));
  return assignees;
}

// Акты назначения concept_process_allocation, размещённые под resultLevel.
std::vector<ScAddr> AllocationActs(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  std::vector<ScAddr> acts;
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(AllocationKeynodes::concept_process_allocation, childAddr, ScType::ConstPermPosArc))
      acts.push_back(childAddr);
  }
  return acts;
}

bool Contains(std::vector<ScAddr> const & addrs, ScAddr const & addr)
{
  return std::find(addrs.begin(), addrs.end(), addr) != addrs.end();
}

// Зарегистрированный в реестре участник: запись со статусом active и
// политикой доступа, разрешающей его. Именно эти два факта агент читает
// как available(s, C) и permit(view(C,s), s, C).
ScAddr MakeRegisteredSystem(ScAgentContext & ctx, bool active = true, bool permitted = true)
{
  ScAddr const systemAddr = ctx.GenerateNode(ScType::ConstNode);
  ScAddr const entryAddr = ctx.GenerateNode(ScType::ConstNode);
  LinkByRelation(ctx, entryAddr, systemAddr, AllocationKeynodes::nrel_entry_participant);

  ScAddr statusAddr = ctx.GenerateNode(ScType::ConstNode);  // произвольный неактивный статус
  if (active)
    statusAddr = AllocationKeynodes::concept_registry_status_active;
  LinkByRelation(ctx, entryAddr, statusAddr, AllocationKeynodes::nrel_entry_status);

  if (permitted)
  {
    ScAddr const policyAddr = ctx.GenerateNode(ScType::ConstNode);
    LinkByRelation(ctx, entryAddr, policyAddr, AllocationKeynodes::nrel_entry_policy);
    LinkByRelation(ctx, policyAddr, systemAddr, AllocationKeynodes::nrel_permits);
  }

  return systemAddr;
}

// Реифицированная оценка применимости пары (σ, s) со значением match.
void MakeAssessment(ScAgentContext & ctx, ScAddr const & subtaskAddr, ScAddr const & systemAddr, std::string const & value)
{
  ScAddr const assessmentAddr = ctx.GenerateNode(ScType::ConstNode);
  ctx.GenerateConnector(ScType::ConstPermPosArc, AllocationKeynodes::concept_match_assessment, assessmentAddr);
  LinkByRole(ctx, assessmentAddr, subtaskAddr, AllocationKeynodes::rrel_subtask);
  LinkByRole(ctx, assessmentAddr, systemAddr, AllocationKeynodes::rrel_assignee);
  LinkByRelation(ctx, assessmentAddr, MakeLink(ctx, value), AllocationKeynodes::nrel_match_value);
}

struct Fixture
{
  ScAddr subtask;
  ScAddr context;
  ScAddr time;
  ScAddr resultLevel;
};

Fixture MakeFixture(ScAgentContext & ctx)
{
  return {
      ctx.GenerateNode(ScType::ConstNode),
      ctx.GenerateNode(ScType::ConstNode),
      MakeLink(ctx, "2026-09-13T10:00:00Z"),
      ctx.GenerateNode(ScType::ConstNode)};
}

void RunAllocation(ScAgentContext & ctx, Fixture const & f, std::string const & theta, std::string const & k)
{
  ScAction action = ctx.GenerateAction(AllocationKeynodes::action_allocate_subtasks);
  action.SetArguments(f.subtask, MakeLink(ctx, theta), MakeLink(ctx, k), f.context, f.time, f.resultLevel);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());
}
}  // namespace

// Числовой пример гл. 2 (таблица применимости): для подзадачи σ1
// кандидаты s1 = 0,895, s2 = 0,828, s4 = 0,608 при θ = 0,70 и
// k(σ1) = 2. Порог преодолевают s1 и s2; s4 отсекается, несмотря на
// приемлемый уровень доверия. Шаг 2.3 листинга даёт χ(σ1) = {s1, s2}.
TEST_F(AgentAllocateSubtasksTest, ReproducesChapterTwoWorkedExample)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  ScAddr const s1 = MakeRegisteredSystem(*m_ctx);
  ScAddr const s2 = MakeRegisteredSystem(*m_ctx);
  ScAddr const s4 = MakeRegisteredSystem(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, s1, "0.895");
  MakeAssessment(*m_ctx, f.subtask, s2, "0.828");
  MakeAssessment(*m_ctx, f.subtask, s4, "0.608");

  RunAllocation(*m_ctx, f, "0.70", "2");

  EXPECT_NEAR(ReadIndicator(*m_ctx, f.resultLevel), 1.0f, 0.0001f);

  std::vector<ScAddr> const assignees = AssigneesOf(*m_ctx, f.subtask);
  ASSERT_EQ(assignees.size(), 2u);
  EXPECT_TRUE(Contains(assignees, s1));
  EXPECT_TRUE(Contains(assignees, s2));
  EXPECT_FALSE(Contains(assignees, s4));

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// Акт назначения реифицирован полностью: шесть ролей, требуемых
// rule_assignment_admissibility и prov_template_assignment. θ попадает
// в rrel_required_confidence, приоритет переносится с подзадачи,
// инициатором указан agent_task_allocator.
TEST_F(AgentAllocateSubtasksTest, ReifiesAllocationActWithAllRoles)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  ScAddr const priorityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, f.subtask, priorityAddr, AllocationKeynodes::nrel_subtask_priority);

  ScAddr const systemAddr = MakeRegisteredSystem(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, systemAddr, "0.92");

  RunAllocation(*m_ctx, f, "0.70", "1");

  std::vector<ScAddr> const acts = AllocationActs(*m_ctx, f.resultLevel);
  ASSERT_EQ(acts.size(), 1u);
  ScAddr const actAddr = acts.front();

  EXPECT_EQ(RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_subtask), f.subtask);
  EXPECT_EQ(RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_assignee), systemAddr);
  EXPECT_EQ(RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_context), f.context);
  EXPECT_EQ(RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_assignment_time), f.time);
  EXPECT_EQ(RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_priority), priorityAddr);
  EXPECT_EQ(RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_initiator), AllocationKeynodes::agent_task_allocator);

  std::string confidence;
  ScAddr const confidenceAddr = RoleTargetOf(*m_ctx, actAddr, AllocationKeynodes::rrel_required_confidence);
  ASSERT_TRUE(m_ctx->IsElement(confidenceAddr));
  EXPECT_TRUE(m_ctx->GetLinkContent(confidenceAddr, confidence));
  EXPECT_EQ(confidence, "0.70");

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// Приоритет не придумывается: у подзадачи без nrel_subtask_priority
// роль rrel_priority на акте не появляется.
TEST_F(AgentAllocateSubtasksTest, OmitsPriorityRoleWhenSubtaskHasNone)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, MakeRegisteredSystem(*m_ctx), "0.92");

  RunAllocation(*m_ctx, f, "0.70", "1");

  std::vector<ScAddr> const acts = AllocationActs(*m_ctx, f.resultLevel);
  ASSERT_EQ(acts.size(), 1u);
  EXPECT_FALSE(m_ctx->IsElement(RoleTargetOf(*m_ctx, acts.front(), AllocationKeynodes::rrel_priority)));

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// available(s, C): участник с неактивной записью реестра недопустим,
// каким бы высоким ни было значение match
// (affirmation_registry_required_for_assignment).
TEST_F(AgentAllocateSubtasksTest, ExcludesSystemWithInactiveRegistryEntry)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  ScAddr const suspendedAddr = MakeRegisteredSystem(*m_ctx, /* active */ false);
  ScAddr const activeAddr = MakeRegisteredSystem(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, suspendedAddr, "0.99");
  MakeAssessment(*m_ctx, f.subtask, activeAddr, "0.75");

  RunAllocation(*m_ctx, f, "0.70", "1");

  std::vector<ScAddr> const assignees = AssigneesOf(*m_ctx, f.subtask);
  ASSERT_EQ(assignees.size(), 1u);
  EXPECT_EQ(assignees.front(), activeAddr);

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// permit(view(C,s), s, C): отсутствие политики, разрешающей участника,
// НЕ означает разрешения по умолчанию -- условие не установлено,
// участник недопустим.
TEST_F(AgentAllocateSubtasksTest, ExcludesSystemWithoutPermittingPolicy)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  ScAddr const unpermittedAddr = MakeRegisteredSystem(*m_ctx, /* active */ true, /* permitted */ false);
  MakeAssessment(*m_ctx, f.subtask, unpermittedAddr, "0.99");

  RunAllocation(*m_ctx, f, "0.70", "1");

  EXPECT_NEAR(ReadIndicator(*m_ctx, f.resultLevel), 0.0f, 0.0001f);
  EXPECT_TRUE(AssigneesOf(*m_ctx, f.subtask).empty());

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// |S_{σ,C}| < k(σ): агент сообщает 0.0 и не строит ни одного назначения.
// Ни повторной декомпозиции, ни отказа он не порождает -- это шаг 2.2
// листинга, область AgentHandleNoAdmissibleAssignment.
TEST_F(AgentAllocateSubtasksTest, ReportsFailureWhenAdmissibleFewerThanMultiplicity)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, MakeRegisteredSystem(*m_ctx), "0.92");

  RunAllocation(*m_ctx, f, "0.70", "2");

  EXPECT_NEAR(ReadIndicator(*m_ctx, f.resultLevel), 0.0f, 0.0001f);
  EXPECT_TRUE(AssigneesOf(*m_ctx, f.subtask).empty());
  EXPECT_TRUE(AllocationActs(*m_ctx, f.resultLevel).empty());

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// Оценки применимости, относящиеся к другой подзадаче, в S_{σ,C} не
// попадают.
TEST_F(AgentAllocateSubtasksTest, IgnoresAssessmentsOfOtherSubtasks)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  ScAddr const otherSubtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  MakeAssessment(*m_ctx, otherSubtaskAddr, MakeRegisteredSystem(*m_ctx), "0.99");

  RunAllocation(*m_ctx, f, "0.70", "1");

  EXPECT_NEAR(ReadIndicator(*m_ctx, f.resultLevel), 0.0f, 0.0001f);
  EXPECT_TRUE(AssigneesOf(*m_ctx, f.subtask).empty());

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// θ -- параметр настройки, а не константа кода: тот же набор кандидатов
// при разных порогах даёт разные множества допустимых.
TEST_F(AgentAllocateSubtasksTest, ThresholdIsTakenFromArgument)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, MakeRegisteredSystem(*m_ctx), "0.608");

  RunAllocation(*m_ctx, f, "0.60", "1");
  EXPECT_NEAR(ReadIndicator(*m_ctx, f.resultLevel), 1.0f, 0.0001f);

  Fixture const g = MakeFixture(*m_ctx);
  MakeAssessment(*m_ctx, g.subtask, MakeRegisteredSystem(*m_ctx), "0.608");

  RunAllocation(*m_ctx, g, "0.70", "1");
  EXPECT_NEAR(ReadIndicator(*m_ctx, g.resultLevel), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// Некорректный порог вне [0,1] -- ошибка, а не подстановка значения.
TEST_F(AgentAllocateSubtasksTest, FailsOnThresholdOutsideUnitInterval)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, MakeRegisteredSystem(*m_ctx), "0.92");

  ScAction action = m_ctx->GenerateAction(AllocationKeynodes::action_allocate_subtasks);
  action.SetArguments(f.subtask, MakeLink(*m_ctx, "1.4"), MakeLink(*m_ctx, "1"), f.context, f.time, f.resultLevel);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_FALSE(action.IsFinishedSuccessfully());

  EXPECT_TRUE(AssigneesOf(*m_ctx, f.subtask).empty());

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}

// Кратность меньше единицы недопустима: χ отображает в НЕПУСТЫЕ
// подмножества участников.
TEST_F(AgentAllocateSubtasksTest, FailsOnMultiplicityBelowOne)
{
  m_ctx->SubscribeAgent<AgentAllocateSubtasks>();

  Fixture const f = MakeFixture(*m_ctx);
  MakeAssessment(*m_ctx, f.subtask, MakeRegisteredSystem(*m_ctx), "0.92");

  ScAction action = m_ctx->GenerateAction(AllocationKeynodes::action_allocate_subtasks);
  action.SetArguments(f.subtask, MakeLink(*m_ctx, "0.70"), MakeLink(*m_ctx, "0"), f.context, f.time, f.resultLevel);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_FALSE(action.IsFinishedSuccessfully());

  m_ctx->UnsubscribeAgent<AgentAllocateSubtasks>();
}
