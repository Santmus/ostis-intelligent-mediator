#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_aggregate_trust.hpp"
#include "keynodes/trust_keynodes.hpp"

using AgentAggregateTrustTest = ScMemoryTest;

namespace
{
ScAddr MakeTrustLevel(ScAgentContext & ctx, float value)
{
  ScAddr const levelAddr = ctx.GenerateNode(ScType::ConstNode);
  ctx.GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, levelAddr);

  ScAddr const valueLinkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(valueLinkAddr, std::to_string(value));
  ctx.GenerateConnector(ScType::ConstPermPosArc, levelAddr, valueLinkAddr);

  return levelAddr;
}
}  // namespace

// Воспроизводит иллюстративный расчёт из scenario_trust_assessment.scs:
// t_rel=0.838, t_acc=0.796, t_sec=0.955, t_tr=0.730,
// lambda=(0.4, 0.25, 0.15, 0.2)
// -> trust = 0.4*0.838 + 0.25*0.796 + 0.15*0.955 + 0.2*0.730 = 0.82345
// (в scenario_trust_assessment.scs округлено до "≈ 0.823").
TEST_F(AgentAggregateTrustTest, AggregatesFourComponentsWithDefaultWeights)
{
  m_ctx->SubscribeAgent<AgentAggregateTrust>();

  ScAddr const relLevelAddr = MakeTrustLevel(*m_ctx, 0.838f);
  ScAddr const accLevelAddr = MakeTrustLevel(*m_ctx, 0.796f);
  ScAddr const secLevelAddr = MakeTrustLevel(*m_ctx, 0.955f);
  ScAddr const trLevelAddr = MakeTrustLevel(*m_ctx, 0.730f);

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_aggregate_trust);
  action.SetArguments(relLevelAddr, accLevelAddr, secLevelAddr, trLevelAddr, trustLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.82345f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentAggregateTrust>();
}

// Компонента без числовой sc-ссылки трактуется как t_k = 0 (нейтральный
// старт), а не как ошибка.
TEST_F(AgentAggregateTrustTest, TreatsMissingComponentValueAsZero)
{
  m_ctx->SubscribeAgent<AgentAggregateTrust>();

  ScAddr const relLevelAddr = MakeTrustLevel(*m_ctx, 1.0f);
  ScAddr const accLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без value_link
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, accLevelAddr);
  ScAddr const secLevelAddr = MakeTrustLevel(*m_ctx, 1.0f);
  ScAddr const trLevelAddr = MakeTrustLevel(*m_ctx, 1.0f);

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_aggregate_trust);
  action.SetArguments(relLevelAddr, accLevelAddr, secLevelAddr, trLevelAddr, trustLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  // trust = 0.4*1 + 0.25*0 + 0.15*1 + 0.2*1 = 0.75
  EXPECT_NEAR(std::stof(content), 0.75f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentAggregateTrust>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentAggregateTrustTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentAggregateTrust>();

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_aggregate_trust);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentAggregateTrust>();
}
