#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_update_trust_security.hpp"
#include "keynodes/trust_keynodes.hpp"

using AgentUpdateTrustSecurityTest = ScMemoryTest;

// t_sec = 0.95, nu_sec = 0 (зарегистрирован инцидент), gamma = 0.10
// -> t_sec_new = 0.9 * 0.95 + 0.10 * 0 = 0.855 -- немедленное снижение
// на очередном шаге обновления.
TEST_F(AgentUpdateTrustSecurityTest, DecreasesTSecOnIncident)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustSecurity>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const initialValueLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(initialValueLinkAddr, std::to_string(0.95f));
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, trustLevelAddr, initialValueLinkAddr);

  ScAddr const incidentLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(incidentLinkAddr, std::to_string(0.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_security);
  action.SetArguments(trustLevelAddr, incidentLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  std::string updatedContent;
  EXPECT_TRUE(m_ctx->GetLinkContent(initialValueLinkAddr, updatedContent));
  EXPECT_NEAR(std::stof(updatedContent), 0.855f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentUpdateTrustSecurity>();
}

// Проверяет случай, когда у trust_level ещё нет числовой sc-ссылки:
// агент должен создать её и принять начальное значение t_sec = 0.
TEST_F(AgentUpdateTrustSecurityTest, CreatesValueLinkWhenMissing)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustSecurity>();

  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAddr const incidentLinkAddr = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(incidentLinkAddr, std::to_string(1.0f));

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_security);
  action.SetArguments(trustLevelAddr, incidentLinkAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const it3 =
      m_ctx->CreateIterator3(trustLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(it3->Next());

  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(it3->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.10f, 0.0001f);  // (1-0.10)*0 + 0.10*1.0

  m_ctx->UnsubscribeAgent<AgentUpdateTrustSecurity>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentUpdateTrustSecurityTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentUpdateTrustSecurity>();

  ScAction action = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_security);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentUpdateTrustSecurity>();
}
