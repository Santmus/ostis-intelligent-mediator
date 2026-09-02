#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_validate_right_to_explanation.hpp"
#include "keynodes/integration_keynodes.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/security_keynodes.hpp"

using AgentValidateRightToExplanationTest = ScMemoryTest;

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

// Ищет среди детей resultLevelAddr узел класса
// concept_property_right_to_explanation (тот же приём, что в
// test_agent_check_acyclicity.cpp).
ScAddr FindViolationChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::Unknown);
  while (it3->Next())
  {
    ScAddr const childAddr = it3->Get(2);
    if (ctx.CheckConnector(
            SecurityKeynodes::concept_property_right_to_explanation, childAddr, ScType::ConstPermPosArc))
      return childAddr;
  }
  return ScAddr::Empty;
}

ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}
}  // namespace

// delivered_to + decision_support + explained_by все присутствуют --
// гейт срабатывает, требование выполнено.
TEST_F(AgentValidateRightToExplanationTest, ValidWhenGateAppliesAndExplanationPresent)
{
  m_ctx->SubscribeAgent<AgentValidateRightToExplanation>();

  ScAddr const irAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const userAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, irAddr, userAddr, SecurityKeynodes::nrel_delivered_to);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, SecurityKeynodes::concept_request_decision_support, requestAddr);
  LinkByRelation(*m_ctx, irAddr, explanationAddr, IntegrationKeynodes::nrel_explained_by);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_validate_right_to_explanation);
  action.SetArguments(irAddr, requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentValidateRightToExplanation>();
}

// delivered_to + recommendation присутствуют, но explained_by
// отсутствует -- гейт срабатывает, требование нарушено; нарушение
// связано с ir через nrel_concerns.
TEST_F(AgentValidateRightToExplanationTest, InvalidWhenGateAppliesAndExplanationMissing)
{
  m_ctx->SubscribeAgent<AgentValidateRightToExplanation>();

  ScAddr const irAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const userAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, irAddr, userAddr, SecurityKeynodes::nrel_delivered_to);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, SecurityKeynodes::concept_request_recommendation, requestAddr);
  // без explained_by

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_validate_right_to_explanation);
  action.SetArguments(irAddr, requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  ScAddr const violationAddr = FindViolationChild(*m_ctx, resultLevelAddr);
  ASSERT_TRUE(m_ctx->IsElement(violationAddr));
  EXPECT_TRUE(m_ctx->CheckConnector(
      SecurityKeynodes::concept_integrity_violation, violationAddr, ScType::ConstPermPosArc));
  EXPECT_EQ(TargetOf(*m_ctx, violationAddr, IntegrityKeynodes::nrel_concerns), irAddr);

  m_ctx->UnsubscribeAgent<AgentValidateRightToExplanation>();
}

// delivered_to отсутствует -- гейт не срабатывает, проверка вырожденно
// проходит, даже без explained_by.
TEST_F(AgentValidateRightToExplanationTest, ValidWhenNotDeliveredRegardlessOfExplanation)
{
  m_ctx->SubscribeAgent<AgentValidateRightToExplanation>();

  ScAddr const irAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без delivered_to
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, SecurityKeynodes::concept_request_decision_support, requestAddr);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_validate_right_to_explanation);
  action.SetArguments(irAddr, requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_FALSE(m_ctx->IsElement(FindViolationChild(*m_ctx, resultLevelAddr)));

  m_ctx->UnsubscribeAgent<AgentValidateRightToExplanation>();
}

// delivered_to присутствует, но запрос — информационный (не
// decision_support/recommendation) -- гейт не срабатывает, проверка
// вырожденно проходит без explained_by.
TEST_F(AgentValidateRightToExplanationTest, ValidWhenRequestTypeDoesNotRequireExplanation)
{
  m_ctx->SubscribeAgent<AgentValidateRightToExplanation>();

  ScAddr const irAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const userAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);  // без decision_support/recommendation
  LinkByRelation(*m_ctx, irAddr, userAddr, SecurityKeynodes::nrel_delivered_to);

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_validate_right_to_explanation);
  action.SetArguments(irAddr, requestAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentValidateRightToExplanation>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentValidateRightToExplanationTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentValidateRightToExplanation>();

  ScAction action = m_ctx->GenerateAction(SecurityKeynodes::action_validate_right_to_explanation);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentValidateRightToExplanation>();
}
