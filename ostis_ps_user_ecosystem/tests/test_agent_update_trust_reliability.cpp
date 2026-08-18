#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_update_trust_reliability.hpp"
#include "keynodes/trust_keynodes.hpp"

using AgentUpdateTrustReliabilityTest = ScMemoryTest;

// Воспроизводит иллюстративный расчёт диссертации (part2.tex,
// eq:trust_rel): t_rel = 0.82, nu_rel = 1, alpha = 0.10
// -> t_rel_new = 0.9 * 0.82 + 0.10 * 1.0 = 0.838.
TEST_F(AgentUpdateTrustReliabilityTest, UpdatesTRelOnSuccessfulOutcome)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustReliability>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const initialValueLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(initialValueLinkAddr, std::to_string(0.82f));
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, initialValueLinkAddr);

  ScAddr const outcomeLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(outcomeLinkAddr, std::to_string(1.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_reliability);
  action.SetArguments(trustLevelAddr, outcomeLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  std::string updatedContent;
  EXPECT_TRUE(m_ctx->GetLinkContent(initialValueLinkAddr, updatedContent));
  EXPECT_NEAR(std::stof(updatedContent), 0.838f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentUpdateTrustReliability>();
}

// Проверяет случай, когда у trust_level ещё нет числовой sc-ссылки:
// агент должен создать её и принять начальное значение t_rel = 0.
TEST_F(AgentUpdateTrustReliabilityTest, CreatesValueLinkWhenMissing)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustReliability>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const outcomeLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(outcomeLinkAddr, std::to_string(1.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_reliability);
  action.SetArguments(trustLevelAddr, outcomeLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.10f, 0.0001f);  // (1-0.10)*0 + 0.10*1.0

  m_ctx->UnsubscribeAgent<AgentUpdateTrustReliability>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentUpdateTrustReliabilityTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustReliability>();

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_reliability);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentUpdateTrustReliability>();
}
