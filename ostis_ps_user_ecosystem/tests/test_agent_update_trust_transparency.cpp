#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_update_trust_transparency.hpp"
#include "keynodes/trust_keynodes.hpp"

using AgentUpdateTrustTransparencyTest = ScMemoryTest;

// t_tr = 0.60, nu_tr = 1 (обоснование предоставлено), delta = 0.10
// -> t_tr_new = 0.9 * 0.60 + 0.10 * 1.0 = 0.64.
TEST_F(AgentUpdateTrustTransparencyTest, UpdatesTTrOnJustificationProvided)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustTransparency>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const initialValueLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(initialValueLinkAddr, std::to_string(0.60f));
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, initialValueLinkAddr);

  ScAddr const justificationLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(justificationLinkAddr, std::to_string(1.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_transparency);
  action.SetArguments(trustLevelAddr, justificationLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  std::string updatedContent;
  EXPECT_TRUE(m_ctx->GetLinkContent(initialValueLinkAddr, updatedContent));
  EXPECT_NEAR(std::stof(updatedContent), 0.64f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentUpdateTrustTransparency>();
}

// Проверяет случай, когда у trust_level ещё нет числовой sc-ссылки:
// агент должен создать её и принять начальное значение t_tr = 0.
TEST_F(AgentUpdateTrustTransparencyTest, CreatesValueLinkWhenMissing)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustTransparency>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const justificationLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(justificationLinkAddr, std::to_string(1.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_transparency);
  action.SetArguments(trustLevelAddr, justificationLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.10f, 0.0001f);  // (1-0.10)*0 + 0.10*1.0

  m_ctx->UnsubscribeAgent<AgentUpdateTrustTransparency>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentUpdateTrustTransparencyTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustTransparency>();

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_transparency);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentUpdateTrustTransparency>();
}
