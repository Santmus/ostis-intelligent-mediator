#include <string>

#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_record_state_transition.hpp"
#include "keynodes/lifecycle_keynodes.hpp"

using AgentRecordStateTransitionTest = ScMemoryTest;

namespace
{
// Локальные копии представлений trust_utils (common/relation_io.hpp,
// common/float_link_io.hpp) -- собраны вручную, как и во всех остальных
// test_agent_*.cpp этого проекта (тесты намеренно не подключают
// production-заголовки common/*.hpp).
void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Цель роли roleAddr, присоединённой к sourceAddr в представлении
// LinkByRole (common/relation_io.hpp): sourceAddr -[ConstPermPosArc]->
// target, дуга классифицирована roleAddr.
ScAddr RoleTarget(ScAgentContext & ctx, ScAddr const & sourceAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(sourceAddr, ScType::ConstPermPosArc, ScType::Unknown, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Источник, присоединивший targetAddr в роли roleAddr (обратный поиск
// относительно RoleTarget) -- находит узел перехода по его rrel_to_state.
ScAddr RoleSource(ScAgentContext & ctx, ScAddr const & targetAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(ScType::Unknown, ScType::ConstPermPosArc, targetAddr, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(0);
  return ScAddr::Empty;
}

void SetResource(ScAgentContext & ctx, ScAddr const & stateAddr, ScAddr const & relationAddr, float value)
{
  ScAddr const linkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(linkAddr, std::to_string(value));
  LinkByRelation(ctx, stateAddr, linkAddr, relationAddr);
}

float ReadResource(ScAgentContext & ctx, ScAddr const & stateAddr, ScAddr const & relationAddr)
{
  ScAddr const linkAddr = TargetOf(ctx, stateAddr, relationAddr);
  EXPECT_TRUE(ctx.IsElement(linkAddr));
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(linkAddr, content));
  return std::stof(content);
}

// Заготовка запроса с текущим состоянием классом fromClassAddr и
// заданными ресурсами; допустимость ребра fromClassAddr -> toClassAddr
// декларируется отдельно (nrel_transitions_to) -- см. каждый тест.
struct RequestFixture
{
  ScAddr requestAddr;
  ScAddr oldStateAddr;
};

RequestFixture MakeRequest(ScAgentContext & ctx, ScAddr const & fromClassAddr, float bRem, float dRem)
{
  RequestFixture fixture;
  fixture.requestAddr = ctx.GenerateNode(ScType::ConstNode);
  fixture.oldStateAddr = ctx.GenerateNode(ScType::ConstNode);
  ctx.GenerateConnector(ScType::ConstPermPosArc, fromClassAddr, fixture.oldStateAddr);
  SetResource(ctx, fixture.oldStateAddr, LifecycleKeynodes::nrel_remaining_reassignment_budget, bRem);
  SetResource(ctx, fixture.oldStateAddr, LifecycleKeynodes::nrel_remaining_replanning_depth, dRem);
  LinkByRelation(ctx, fixture.requestAddr, fixture.oldStateAddr, LifecycleKeynodes::nrel_has_state);
  return fixture;
}
}  // namespace

// x_exec -> x_aggregate: ребро допустимо и не является ребром
// переназначения -- оба ресурса переносятся в новый экземпляр состояния
// без изменений, полный пакет перехода (constraint_transition_provenance)
// построен целиком.
TEST_F(AgentRecordStateTransitionTest, TransitionsExecutionToAggregationCarryingResourcesUnchanged)
{
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();

  LinkByRelation(
      *m_ctx, LifecycleKeynodes::concept_state_execution, LifecycleKeynodes::concept_state_aggregation,
      LifecycleKeynodes::nrel_transitions_to);

  RequestFixture const fixture = MakeRequest(*m_ctx, LifecycleKeynodes::concept_state_execution, 3.0f, 2.0f);
  ScAddr const eventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const planVersionAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  action.SetArguments(fixture.requestAddr, LifecycleKeynodes::concept_state_aggregation, eventAddr, planVersionAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const newStateAddr = TargetOf(*m_ctx, fixture.requestAddr, LifecycleKeynodes::nrel_has_state);
  ASSERT_TRUE(m_ctx->IsElement(newStateAddr));
  EXPECT_NE(newStateAddr, fixture.oldStateAddr);
  EXPECT_TRUE(
      m_ctx->CreateIterator3(LifecycleKeynodes::concept_state_aggregation, ScType::ConstPermPosArc, newStateAddr)
          ->Next());
  EXPECT_NEAR(
      ReadResource(*m_ctx, newStateAddr, LifecycleKeynodes::nrel_remaining_reassignment_budget), 3.0f, 0.0001f);
  EXPECT_NEAR(
      ReadResource(*m_ctx, newStateAddr, LifecycleKeynodes::nrel_remaining_replanning_depth), 2.0f, 0.0001f);

  ScAddr const transitionAddr = RoleSource(*m_ctx, newStateAddr, LifecycleKeynodes::rrel_to_state);
  ASSERT_TRUE(m_ctx->IsElement(transitionAddr));
  EXPECT_TRUE(
      m_ctx->CreateIterator3(LifecycleKeynodes::concept_state_transition, ScType::ConstPermPosArc, transitionAddr)
          ->Next());
  EXPECT_EQ(RoleTarget(*m_ctx, transitionAddr, LifecycleKeynodes::rrel_from_state), fixture.oldStateAddr);
  EXPECT_EQ(RoleTarget(*m_ctx, transitionAddr, LifecycleKeynodes::rrel_transition_event), eventAddr);
  EXPECT_EQ(TargetOf(*m_ctx, transitionAddr, LifecycleKeynodes::nrel_caused_by_event), eventAddr);
  EXPECT_EQ(TargetOf(*m_ctx, transitionAddr, LifecycleKeynodes::nrel_uses_plan_version), planVersionAddr);

  ScAddr const auditRecordAddr = TargetOf(*m_ctx, transitionAddr, LifecycleKeynodes::nrel_transition_recorded_in);
  ASSERT_TRUE(m_ctx->IsElement(auditRecordAddr));
  EXPECT_TRUE(
      m_ctx->CreateIterator3(LifecycleKeynodes::concept_audit_record, ScType::ConstPermPosArc, auditRecordAddr)
          ->Next());
  EXPECT_TRUE(m_ctx->CreateIterator3(
                  LifecycleKeynodes::concept_audit_record_transition, ScType::ConstPermPosArc, auditRecordAddr)
                  ->Next());
  EXPECT_TRUE(m_ctx->IsElement(TargetOf(*m_ctx, auditRecordAddr, LifecycleKeynodes::nrel_has_provenance)));

  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
}

// x_aggregate -> x_verify: тот же не-ресурсный случай на другом ребре
// уже покрытого агентами участка конвейера.
TEST_F(AgentRecordStateTransitionTest, TransitionsAggregationToVerificationCarryingResourcesUnchanged)
{
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();

  LinkByRelation(
      *m_ctx, LifecycleKeynodes::concept_state_aggregation, LifecycleKeynodes::concept_state_verification,
      LifecycleKeynodes::nrel_transitions_to);

  RequestFixture const fixture = MakeRequest(*m_ctx, LifecycleKeynodes::concept_state_aggregation, 1.0f, 0.0f);
  ScAddr const eventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const planVersionAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  action.SetArguments(fixture.requestAddr, LifecycleKeynodes::concept_state_verification, eventAddr, planVersionAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const newStateAddr = TargetOf(*m_ctx, fixture.requestAddr, LifecycleKeynodes::nrel_has_state);
  ASSERT_TRUE(m_ctx->IsElement(newStateAddr));
  EXPECT_TRUE(
      m_ctx->CreateIterator3(LifecycleKeynodes::concept_state_verification, ScType::ConstPermPosArc, newStateAddr)
          ->Next());
  EXPECT_NEAR(
      ReadResource(*m_ctx, newStateAddr, LifecycleKeynodes::nrel_remaining_reassignment_budget), 1.0f, 0.0001f);
  EXPECT_NEAR(
      ReadResource(*m_ctx, newStateAddr, LifecycleKeynodes::nrel_remaining_replanning_depth), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
}

// x_exec -> x_assign: единственное ребро Фазы 1, расходующее ресурс --
// b_rem уменьшается на 1, d_rem не меняется
// (constraint_separate_resource_decrease).
TEST_F(AgentRecordStateTransitionTest, TransitionsExecutionToAssignmentDecrementingReassignmentBudget)
{
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();

  LinkByRelation(
      *m_ctx, LifecycleKeynodes::concept_state_execution, LifecycleKeynodes::concept_state_assignment,
      LifecycleKeynodes::nrel_transitions_to);

  RequestFixture const fixture = MakeRequest(*m_ctx, LifecycleKeynodes::concept_state_execution, 2.0f, 1.0f);
  ScAddr const eventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const planVersionAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  action.SetArguments(fixture.requestAddr, LifecycleKeynodes::concept_state_assignment, eventAddr, planVersionAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const newStateAddr = TargetOf(*m_ctx, fixture.requestAddr, LifecycleKeynodes::nrel_has_state);
  ASSERT_TRUE(m_ctx->IsElement(newStateAddr));
  EXPECT_NEAR(
      ReadResource(*m_ctx, newStateAddr, LifecycleKeynodes::nrel_remaining_reassignment_budget), 1.0f, 0.0001f);
  EXPECT_NEAR(
      ReadResource(*m_ctx, newStateAddr, LifecycleKeynodes::nrel_remaining_replanning_depth), 1.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
}

// То же ребро x_exec -> x_assign, но b_rem уже 0 -- отказ, факты не
// создаются, request остаётся на прежнем состоянии.
TEST_F(AgentRecordStateTransitionTest, FinishesWithErrorWhenReassignmentBudgetExhausted)
{
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();

  LinkByRelation(
      *m_ctx, LifecycleKeynodes::concept_state_execution, LifecycleKeynodes::concept_state_assignment,
      LifecycleKeynodes::nrel_transitions_to);

  RequestFixture const fixture = MakeRequest(*m_ctx, LifecycleKeynodes::concept_state_execution, 0.0f, 1.0f);
  ScAddr const eventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const planVersionAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  action.SetArguments(fixture.requestAddr, LifecycleKeynodes::concept_state_assignment, eventAddr, planVersionAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  EXPECT_EQ(TargetOf(*m_ctx, fixture.requestAddr, LifecycleKeynodes::nrel_has_state), fixture.oldStateAddr);

  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
}

// x_exec -> x_verify напрямую: нет соответствующего ребра в
// nrel_transitions_to (граф допустимости mediation_lifecycle.scs не
// включает такое ребро) -- отказ.
TEST_F(AgentRecordStateTransitionTest, FinishesWithErrorOnInadmissibleTransition)
{
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();

  RequestFixture const fixture = MakeRequest(*m_ctx, LifecycleKeynodes::concept_state_execution, 2.0f, 1.0f);
  ScAddr const eventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const planVersionAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  action.SetArguments(fixture.requestAddr, LifecycleKeynodes::concept_state_verification, eventAddr, planVersionAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  EXPECT_EQ(TargetOf(*m_ctx, fixture.requestAddr, LifecycleKeynodes::nrel_has_state), fixture.oldStateAddr);

  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentRecordStateTransitionTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();

  ScAction action = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
}
