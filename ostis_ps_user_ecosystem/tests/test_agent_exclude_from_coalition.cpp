#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_exclude_from_coalition.hpp"
#include "keynodes/coalition_keynodes.hpp"

using AgentExcludeFromCoalitionTest = ScMemoryTest;

namespace
{
void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

float ReadIndicator(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
}

bool IsMember(ScAgentContext & ctx, ScAddr const & systemAddr, ScAddr const & communityAddr)
{
  ScIterator5Ptr const it5 = ctx.CreateIterator5(
      systemAddr, ScType::ConstCommonArc, communityAddr, ScType::ConstPermPosArc, CoalitionKeynodes::nrel_member_of);
  return it5->Next();
}
}  // namespace

// failed + assigned_to + member_of все выполнены -- система исключается
// из коалиции, индикатор 1.0.
TEST_F(AgentExcludeFromCoalitionTest, ExcludesSystemWhenAllConditionsHold)
{
  m_ctx->SubscribeAgent<AgentExcludeFromCoalition>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const communityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, systemAddr, communityAddr, CoalitionKeynodes::nrel_member_of);

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, CoalitionKeynodes::nrel_assigned_to);
  LinkByRelation(
      *m_ctx, subtaskAddr, CoalitionKeynodes::concept_subtask_status_failed, CoalitionKeynodes::nrel_subtask_status);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_exclude_from_coalition);
  action.SetArguments(subtaskAddr, communityAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(IsMember(*m_ctx, systemAddr, communityAddr));

  m_ctx->UnsubscribeAgent<AgentExcludeFromCoalition>();
}

// Подзадача не failed -- условие не выполнено, членство сохраняется,
// индикатор 0.0.
TEST_F(AgentExcludeFromCoalitionTest, DoesNotExcludeWhenSubtaskNotFailed)
{
  m_ctx->SubscribeAgent<AgentExcludeFromCoalition>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const communityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, systemAddr, communityAddr, CoalitionKeynodes::nrel_member_of);

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, CoalitionKeynodes::nrel_assigned_to);
  LinkByRelation(
      *m_ctx, subtaskAddr, CoalitionKeynodes::concept_subtask_status_reassigned,
      CoalitionKeynodes::nrel_subtask_status);  // не failed

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_exclude_from_coalition);
  action.SetArguments(subtaskAddr, communityAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);
  EXPECT_TRUE(IsMember(*m_ctx, systemAddr, communityAddr));

  m_ctx->UnsubscribeAgent<AgentExcludeFromCoalition>();
}

// failed + assigned_to выполнены, но система не состоит именно в
// переданной community -- условие member_of(s,K) не выполнено,
// индикатор 0.0, ничего не стирается.
TEST_F(AgentExcludeFromCoalitionTest, DoesNotExcludeWhenNotMemberOfGivenCommunity)
{
  m_ctx->SubscribeAgent<AgentExcludeFromCoalition>();

  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const otherCommunityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, systemAddr, otherCommunityAddr, CoalitionKeynodes::nrel_member_of);
  ScAddr const unrelatedCommunityAddr = m_ctx->GenerateNode(ScType::ConstNode);  // передаётся агенту

  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, CoalitionKeynodes::nrel_assigned_to);
  LinkByRelation(
      *m_ctx, subtaskAddr, CoalitionKeynodes::concept_subtask_status_failed, CoalitionKeynodes::nrel_subtask_status);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_exclude_from_coalition);
  action.SetArguments(subtaskAddr, unrelatedCommunityAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);
  EXPECT_TRUE(IsMember(*m_ctx, systemAddr, otherCommunityAddr));  // членство в реальной community не тронуто

  m_ctx->UnsubscribeAgent<AgentExcludeFromCoalition>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentExcludeFromCoalitionTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentExcludeFromCoalition>();

  ScAction action = m_ctx->GenerateAction(CoalitionKeynodes::action_exclude_from_coalition);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentExcludeFromCoalition>();
}
