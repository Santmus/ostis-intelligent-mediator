#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_evaluate_progressive_disclosure.hpp"
#include "keynodes/adaptive_interface_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using AgentEvaluateProgressiveDisclosureTest = ScMemoryTest;

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
}  // namespace

// Цепочка derived_from длиной 5 шагов (> 4) от explanation, группа
// частичных результатов пуста -- срабатывает только условие глубины.
TEST_F(AgentEvaluateProgressiveDisclosureTest, TriggersOnDeepProvenanceChain)
{
  m_ctx->SubscribeAgent<AgentEvaluateProgressiveDisclosure>();

  ScAddr nodes[6];
  for (auto & node : nodes)
    node = m_ctx->GenerateNode(ScType::ConstNode);
  for (size_t i = 0; i + 1 < 6; ++i)
    LinkByRelation(*m_ctx, nodes[i], nodes[i + 1], ProvenanceKeynodes::nrel_derived_from);  // 5 шагов от nodes[0]

  ScAddr const partialResultsGroupAddr = m_ctx->GenerateNode(ScType::ConstNode);  // пусто
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_progressive_disclosure);
  action.SetArguments(nodes[0], partialResultsGroupAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentEvaluateProgressiveDisclosure>();
}

// Цепочка derived_from длиной 2 шага (<= 4), группа из 6 частичных
// результатов (> 5) -- срабатывает только условие количества.
TEST_F(AgentEvaluateProgressiveDisclosureTest, TriggersOnManyPartialResults)
{
  m_ctx->SubscribeAgent<AgentEvaluateProgressiveDisclosure>();

  ScAddr const explanationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const midAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const leafAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, explanationAddr, midAddr, ProvenanceKeynodes::nrel_derived_from);
  LinkByRelation(*m_ctx, midAddr, leafAddr, ProvenanceKeynodes::nrel_derived_from);  // глубина 2

  ScAddr const partialResultsGroupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  for (int i = 0; i < 6; ++i)
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, partialResultsGroupAddr, m_ctx->GenerateNode(ScType::ConstNode));

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_progressive_disclosure);
  action.SetArguments(explanationAddr, partialResultsGroupAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 1.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentEvaluateProgressiveDisclosure>();
}

// Ни глубина, ни количество не превышают порогов -- раскрытие не
// требуется.
TEST_F(AgentEvaluateProgressiveDisclosureTest, DoesNotTriggerWhenBothBelowThreshold)
{
  m_ctx->SubscribeAgent<AgentEvaluateProgressiveDisclosure>();

  ScAddr const explanationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const leafAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, explanationAddr, leafAddr, ProvenanceKeynodes::nrel_derived_from);  // глубина 1

  ScAddr const partialResultsGroupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  for (int i = 0; i < 3; ++i)
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, partialResultsGroupAddr, m_ctx->GenerateNode(ScType::ConstNode));

  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_progressive_disclosure);
  action.SetArguments(explanationAddr, partialResultsGroupAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_NEAR(ReadIndicator(*m_ctx, resultLevelAddr), 0.0f, 0.0001f);

  m_ctx->UnsubscribeAgent<AgentEvaluateProgressiveDisclosure>();
}

// Цикл в derived_from (r1 -> r2 -> r1) -- глубина обязана вычислиться
// без зависания (защита onStack), сама по себе часть проверки.
TEST_F(AgentEvaluateProgressiveDisclosureTest, HandlesCycleInDerivedFromWithoutHanging)
{
  m_ctx->SubscribeAgent<AgentEvaluateProgressiveDisclosure>();

  ScAddr const r1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const r2 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, r1, r2, ProvenanceKeynodes::nrel_derived_from);
  LinkByRelation(*m_ctx, r2, r1, ProvenanceKeynodes::nrel_derived_from);  // замыкает цикл

  ScAddr const partialResultsGroupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const resultLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_progressive_disclosure);
  action.SetArguments(r1, partialResultsGroupAddr, resultLevelAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));  // не зависает -- само по себе часть проверки
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  m_ctx->UnsubscribeAgent<AgentEvaluateProgressiveDisclosure>();
}

// Проверяет, что действие с некорректными (пустыми) аргументами
// завершается с ошибкой, а не молча.
TEST_F(AgentEvaluateProgressiveDisclosureTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentEvaluateProgressiveDisclosure>();

  ScAction action = m_ctx->GenerateAction(AdaptiveInterfaceKeynodes::action_evaluate_progressive_disclosure);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentEvaluateProgressiveDisclosure>();
}
