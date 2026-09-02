#include <vector>

#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_link_explanation_to_provenance.hpp"
#include "keynodes/explanation_keynodes.hpp"
#include "keynodes/provenance_keynodes.hpp"

using AgentLinkExplanationToProvenanceTest = ScMemoryTest;

namespace
{
// Присоединяет targetAddr к sourceAddr в роли roleAddr -- то же
// представление, что LinkByRole (common/relation_io.hpp), но собрано
// вручную, как и в test_agent_build_explanation.cpp.
void LinkByRole(ScAgentContext & ctx, ScAddr const & sourceAddr, ScAddr const & targetAddr, ScAddr const & roleAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstPermPosArc, sourceAddr, targetAddr);
  ctx.GenerateConnector(ScType::ConstPermPosArc, roleAddr, arcAddr);
}

// Все цели заданной нероле-вой связи (nrel_*) sourceAddr -[relationAddr]-> ?.
std::vector<ScAddr> RelationTargets(ScAgentContext & ctx, ScAddr const & sourceAddr, ScAddr const & relationAddr)
{
  std::vector<ScAddr> targets;
  ScIterator5Ptr const it5 = ctx.CreateIterator5(
      sourceAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  while (it5->Next())
    targets.push_back(it5->Get(2));
  return targets;
}

bool Contains(std::vector<ScAddr> const & haystack, ScAddr const & needle)
{
  for (ScAddr const & addr : haystack)
    if (addr == needle)
      return true;
  return false;
}
}  // namespace

// Акт построения объяснения с двумя use_results и одним applies_rules --
// wasDerivedFrom создаётся для каждого использованного результата,
// PROV:used -- для применённого правила, wasAttributedTo -- один раз
// на builder_prov_agent.
TEST_F(AgentLinkExplanationToProvenanceTest, LinksAllUsedResultsAppliedRulesAndAttribution)
{
  m_ctx->SubscribeAgent<AgentLinkExplanationToProvenance>();

  ScAddr const constructionActAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, ExplanationKeynodes::concept_explanation_construction_act, constructionActAddr);

  ScAddr const partialResult1 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const partialResult2 = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRole(*m_ctx, constructionActAddr, partialResult1, ExplanationKeynodes::rrel_uses_results);
  LinkByRole(*m_ctx, constructionActAddr, partialResult2, ExplanationKeynodes::rrel_uses_results);

  ScAddr const aggregationRule = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRole(*m_ctx, constructionActAddr, aggregationRule, ExplanationKeynodes::rrel_applies_rules);

  ScAddr const explanationEntityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationActivityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const builderProvAgentAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_link_explanation_to_provenance);
  action.SetArguments(constructionActAddr, explanationEntityAddr, explanationActivityAddr, builderProvAgentAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  std::vector<ScAddr> const derivedFrom =
      RelationTargets(*m_ctx, explanationEntityAddr, ProvenanceKeynodes::nrel_prov_was_derived_from);
  EXPECT_TRUE(Contains(derivedFrom, partialResult1));
  EXPECT_TRUE(Contains(derivedFrom, partialResult2));
  EXPECT_EQ(derivedFrom.size(), 2u);

  std::vector<ScAddr> const used =
      RelationTargets(*m_ctx, explanationActivityAddr, ProvenanceKeynodes::nrel_prov_used);
  ASSERT_EQ(used.size(), 1u);
  EXPECT_EQ(used[0], aggregationRule);

  std::vector<ScAddr> const attributedTo =
      RelationTargets(*m_ctx, explanationEntityAddr, ProvenanceKeynodes::nrel_prov_was_attributed_to);
  ASSERT_EQ(attributedTo.size(), 1u);
  EXPECT_EQ(attributedTo[0], builderProvAgentAddr);

  ScStructure const resultStruct = action.GetResult();
  EXPECT_TRUE(resultStruct.Has(explanationEntityAddr));
  EXPECT_TRUE(resultStruct.Has(explanationActivityAddr));

  m_ctx->UnsubscribeAgent<AgentLinkExplanationToProvenance>();
}

// construction_act без целей rrel_uses_results/rrel_applies_rules (как у
// counterfactual-объяснения, использующего вместо этого rrel_based_on) --
// оба цикла ничего не пишут, но wasAttributedTo всё равно
// устанавливается, и это не ошибка (тот же принцип "нет данных =
// ничего не пишем", что и в AgentBuildExplanation).
TEST_F(AgentLinkExplanationToProvenanceTest, WritesOnlyAttributionWhenRoleTargetsAreAbsent)
{
  m_ctx->SubscribeAgent<AgentLinkExplanationToProvenance>();

  ScAddr const constructionActAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, ExplanationKeynodes::concept_explanation_construction_act, constructionActAddr);

  ScAddr const explanationEntityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationActivityAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const builderProvAgentAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_link_explanation_to_provenance);
  action.SetArguments(constructionActAddr, explanationEntityAddr, explanationActivityAddr, builderProvAgentAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  EXPECT_TRUE(RelationTargets(*m_ctx, explanationEntityAddr, ProvenanceKeynodes::nrel_prov_was_derived_from).empty());
  EXPECT_TRUE(RelationTargets(*m_ctx, explanationActivityAddr, ProvenanceKeynodes::nrel_prov_used).empty());

  std::vector<ScAddr> const attributedTo =
      RelationTargets(*m_ctx, explanationEntityAddr, ProvenanceKeynodes::nrel_prov_was_attributed_to);
  ASSERT_EQ(attributedTo.size(), 1u);
  EXPECT_EQ(attributedTo[0], builderProvAgentAddr);

  m_ctx->UnsubscribeAgent<AgentLinkExplanationToProvenance>();
}

// Проверяет, что действие без обязательных аргументов завершается с
// ошибкой, а не молча.
TEST_F(AgentLinkExplanationToProvenanceTest, FinishesWithErrorOnMissingArguments)
{
  m_ctx->SubscribeAgent<AgentLinkExplanationToProvenance>();

  ScAction action = m_ctx->GenerateAction(ProvenanceKeynodes::action_link_explanation_to_provenance);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentLinkExplanationToProvenance>();
}
