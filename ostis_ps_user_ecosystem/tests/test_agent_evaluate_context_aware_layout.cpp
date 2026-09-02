#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_evaluate_context_aware_layout.hpp"
#include "keynodes/adaptive_interface_keynodes.hpp"

using AgentEvaluateContextAwareLayoutTest = ScMemoryTest;

namespace
{
float ReadIndicator(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(resultLevelAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
}

bool HasStrategyChild(ScAgentContext & ctx, ScAddr const & resultLevelAddr)
{
  return ctx.CheckConnector(
      resultLevelAddr, AdaptiveInterfaceKeynodes::concept_strategy_context_aware_layout, ScType::ConstPermPosArc);
}
}  // namespace

// situation классифицирован concept_night_shift_context -- срабатывает.
TEST_F(AgentEvaluateContextAwareLayoutTest, TriggersOnNightShift)
{
  m_ctx->SubscribeAgent<AgentEvaluateContextAwareLayout>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_night_shift_context, situationAddr);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_context_aware_layout);
  action.SetArguments(situationAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_TRUE(HasStrategyChild(*m_ctx, resultLevelAddr));

  m_ctx->UnsubscribeAgent<AgentEvaluateContextAwareLayout>();
}

// situation классифицирован concept_low_light_context -- срабатывает
// (второе, независимое условие ветвления).
TEST_F(AgentEvaluateContextAwareLayoutTest, TriggersOnLowLight)
{
  m_ctx->SubscribeAgent<AgentEvaluateContextAwareLayout>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, AdaptiveInterfaceKeynodes::concept_low_light_context, situationAddr);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_context_aware_layout);
  action.SetArguments(situationAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);
  EXPECT_TRUE(HasStrategyChild(*m_ctx, resultLevelAddr));

  m_ctx->UnsubscribeAgent<AgentEvaluateContextAwareLayout>();
}

// Ни один из двух флагов не установлен -- не срабатывает, маркер
// стратегии не присоединяется.
TEST_F(AgentEvaluateContextAwareLayoutTest, DoesNotTriggerWithoutEitherFlag)
{
  m_ctx->SubscribeAgent<AgentEvaluateContextAwareLayout>();

  ScAddr const situationAddr = m_ctx->GenerateNode(ScType::ConstNode);  // ни night_shift, ни low_light
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_context_aware_layout);
  action.SetArguments(situationAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);
  EXPECT_FALSE(HasStrategyChild(*m_ctx, resultLevelAddr));

  m_ctx->UnsubscribeAgent<AgentEvaluateContextAwareLayout>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentEvaluateContextAwareLayoutTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentEvaluateContextAwareLayout>();

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_context_aware_layout);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentEvaluateContextAwareLayout>();
}
