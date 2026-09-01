#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_detect_anomalous_request_rate.hpp"
#include "keynodes/security_keynodes.hpp"

using AgentDetectAnomalousRequestRateTest = ScMemoryTest;

namespace
{
ScAddr MakeFloatLink(ScAgentContext & ctx, float value)
{
  ScAddr const linkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(linkAddr, std::to_string(value));
  return linkAddr;
}

// Ищет среди детей resultLevelAddr узел класса concept_security_incident,
// не полагаясь на различение sc-ссылки/узла типом в запросе итератора.
// Возвращает ScAddr::Empty, если такого инцидента нет.
ScAddr FindIncidentChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(SecurityKeynodes::concept_security_incident, childAddr, ScType::ConstPermPosArc))
      return childAddr;
  }
  return ScAddr::Empty;
}
}  // namespace

// Воспроизводит иллюстративный пример из scenario_security_breach_response.scs:
// rate=5400, baseline=200 (5400/200=27, "превышает базовую линию в 27 раз"),
// k_sigma=5 -> 27 > 5 -> аномалия обнаружена, создаётся concept_security_incident.
TEST_F(AgentDetectAnomalousRequestRateTest, DetectsAnomalyAndCreatesIncident)
{
  m_ctx->SubscribeAgent<AgentDetectAnomalousRequestRate>();

  ScAddr const rateAddr = MakeFloatLink(*m_ctx, 5400.0f);
  ScAddr const baselineAddr = MakeFloatLink(*m_ctx, 200.0f);
  ScAddr const kSigmaAddr = MakeFloatLink(*m_ctx, 5.0f);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_detect_anomalous_request_rate);
  action.SetArguments(rateAddr, baselineAddr, kSigmaAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const valueIt =
      m_ctx->CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(valueIt->Next());
  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(valueIt->Get(2), content));
  EXPECT_NEAR(std::stof(content), 1.0f, 0.0001f);

  ScAddr const incidentAddr = FindIncidentChild(*m_ctx, resultLevelAddr);
  ASSERT_TRUE(m_ctx->IsElement(incidentAddr));

  // Чисто объёмный сигнал (rate/baseline) обосновывает только DoS,
  // не Tampering (AGENTS_AUDIT.md п.6.9).
  EXPECT_TRUE(m_ctx->CheckConnector(
      SecurityKeynodes::concept_incident_denial_of_service, incidentAddr, ScType::ConstPermPosArc));

  m_ctx->UnsubscribeAgent<AgentDetectAnomalousRequestRate>();
}

// Частота в пределах нормы (rate=250, baseline=200, k_sigma=5 -> 1.25 <= 5) --
// не аномалия, инцидент не создаётся.
TEST_F(AgentDetectAnomalousRequestRateTest, DoesNotFlagNormalTraffic)
{
  m_ctx->SubscribeAgent<AgentDetectAnomalousRequestRate>();

  ScAddr const rateAddr = MakeFloatLink(*m_ctx, 250.0f);
  ScAddr const baselineAddr = MakeFloatLink(*m_ctx, 200.0f);
  ScAddr const kSigmaAddr = MakeFloatLink(*m_ctx, 5.0f);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_detect_anomalous_request_rate);
  action.SetArguments(rateAddr, baselineAddr, kSigmaAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScIterator3Ptr const valueIt =
      m_ctx->CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  ASSERT_TRUE(valueIt->Next());
  std::string content;
  EXPECT_TRUE(m_ctx->GetLinkContent(valueIt->Get(2), content));
  EXPECT_NEAR(std::stof(content), 0.0f, 0.0001f);

  EXPECT_FALSE(m_ctx->IsElement(FindIncidentChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentDetectAnomalousRequestRate>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentDetectAnomalousRequestRateTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentDetectAnomalousRequestRate>();

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_detect_anomalous_request_rate);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentDetectAnomalousRequestRate>();
}
