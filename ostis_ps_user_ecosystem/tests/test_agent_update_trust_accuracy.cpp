#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_update_trust_accuracy.hpp"
#include "keynodes/trust_keynodes.hpp"

using AgentUpdateTrustAccuracyTest = ScMemoryTest;

// t_acc = 0.70, q_res = 0.90, beta = 0.10
// -> t_acc_new = 0.9 * 0.70 + 0.10 * 0.90 = 0.72.
TEST_F(AgentUpdateTrustAccuracyTest, UpdatesTAccOnQualityResult)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustAccuracy>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const initialValueLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(initialValueLinkAddr, std::to_string(0.70f));
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, initialValueLinkAddr);

  ScAddr const qualityLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(qualityLinkAddr, std::to_string(0.90f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_accuracy);
  action.SetArguments(trustLevelAddr, qualityLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  std::string updatedContent;
  EXPECT_TRUE(m_ctx->GetLinkContent(initialValueLinkAddr, updatedContent));
  EXPECT_NEAR(std::stof(updatedContent), 0.72f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentUpdateTrustAccuracy>();
}

// Проверяет случай, когда у trust_level ещё нет числовой sc-ссылки:
// агент должен создать её и принять начальное значение t_acc = 0.
TEST_F(AgentUpdateTrustAccuracyTest, CreatesValueLinkWhenMissing)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustAccuracy>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const qualityLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(qualityLinkAddr, std::to_string(1.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_accuracy);
  action.SetArguments(trustLevelAddr, qualityLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.10f, 0.0001f);  // (1-0.10)*0 + 0.10*1.0

  m_ctx->UnsubscribeAgent<AgentUpdateTrustAccuracy>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentUpdateTrustAccuracyTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustAccuracy>();

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_accuracy);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentUpdateTrustAccuracy>();
}
