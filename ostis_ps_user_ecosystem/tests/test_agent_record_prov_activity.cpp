#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_record_prov_activity.hpp"
#include "keynodes/provenance_keynodes.hpp"

using AgentRecordProvActivityTest = ScMemoryTest;

namespace
{
ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Единственная цель заданной роли (rrel_*) из actAddr, либо
// ScAddr::Empty (тот же приём, что в test_agent_build_explanation.cpp).
ScAddr RoleTarget(ScAgentContext & ctx, ScAddr const & actAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(actAddr, ScType::ConstPermPosArc, ScType::Unknown, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Находит овеществлённую PROV Activity по ролевой связи
// activity -[rrel_input]-> targetAddr (тот же приём, что
// FindActByRoleTarget в test_agent_build_explanation.cpp).
ScAddr FindActivityByInput(ScAgentContext & ctx, ScAddr const & inputAddr)
{
  ScIterator5Ptr const it5 = ctx.CreateIterator5(
      ScType::ConstNode, ScType::ConstPermPosArc, inputAddr, ScType::ConstPermPosArc,
      ProvenanceKeynodes::rrel_input);
  if (it5->Next())
    return it5->Get(0);
  return ScAddr::Empty;
}
}  // namespace

// Базовый случай с меткой времени: создаётся PROV Activity с ролями
// actor/input/timestamp, PROV:used и wasAssociatedWith.
TEST_F(AgentRecordProvActivityTest, RecordsActivityWithAllRolesAndTimestamp)
{
  m_ctx->SubscribeAgent<AgentRecordProvActivity>();

  ScAddr const actorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const inputAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const provAgentAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const timestampAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_activity);
  action.SetArguments(actorAddr, inputAddr, provAgentAddr, timestampAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const activityAddr = FindActivityByInput(*m_ctx, inputAddr);
  ASSERT_TRUE(m_ctx->IsElement(activityAddr));
  EXPECT_TRUE(
      m_ctx->CheckConnector(ProvenanceKeynodes::concept_prov_activity, activityAddr, ScType::ConstPermPosArc));

  EXPECT_EQ(RoleTarget(*m_ctx, activityAddr, ProvenanceKeynodes::rrel_actor), actorAddr);
  EXPECT_EQ(RoleTarget(*m_ctx, activityAddr, ProvenanceKeynodes::rrel_timestamp), timestampAddr);
  EXPECT_EQ(TargetOf(*m_ctx, activityAddr, ProvenanceKeynodes::nrel_prov_used), inputAddr);
  EXPECT_EQ(TargetOf(*m_ctx, activityAddr, ProvenanceKeynodes::nrel_prov_was_associated_with), provAgentAddr);

  ScStructure const resultStruct = action.GetResult();
  EXPECT_TRUE(resultStruct.Has(activityAddr));
  EXPECT_TRUE(resultStruct.Has(inputAddr));

  m_ctx->UnsubscribeAgent<AgentRecordProvActivity>();
}

// timestamp -- необязательный (последний) аргумент: без него роль
// rrel_timestamp просто отсутствует, остальное записывается как обычно.
TEST_F(AgentRecordProvActivityTest, OmitsTimestampRoleWhenArgumentMissing)
{
  m_ctx->SubscribeAgent<AgentRecordProvActivity>();

  ScAddr const actorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const inputAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const provAgentAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_activity);
  action.SetArguments(actorAddr, inputAddr, provAgentAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const activityAddr = FindActivityByInput(*m_ctx, inputAddr);
  ASSERT_TRUE(m_ctx->IsElement(activityAddr));
  EXPECT_EQ(RoleTarget(*m_ctx, activityAddr, ProvenanceKeynodes::rrel_actor), actorAddr);
  EXPECT_FALSE(m_ctx->IsElement(RoleTarget(*m_ctx, activityAddr, ProvenanceKeynodes::rrel_timestamp)));

  m_ctx->UnsubscribeAgent<AgentRecordProvActivity>();
}

// Проверяет, что действие без обязательных аргументов завершается с
// ошибкой, а не молча.
TEST_F(AgentRecordProvActivityTest, FinishesWithErrorOnMissingRequiredArguments)
{
  m_ctx->SubscribeAgent<AgentRecordProvActivity>();

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_record_prov_activity);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentRecordProvActivity>();
}
