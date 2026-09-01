#include <vector>

#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_build_explanation.hpp"
#include "keynodes/explanation_keynodes.hpp"

using AgentBuildExplanationTest = ScMemoryTest;

namespace
{
// Единственная цель nrel_supported_by_explanation/nrel_explained_by
// от sourceAddr, либо ScAddr::Empty, если связи нет.
ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & sourceAddr, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 = ctx.CreateIterator5(
      sourceAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Находит овеществлённый акт построения объяснения по ролевой связи
// act -[roleAddr]-> targetAddr (rrel_explanandum -> explanandumAddr).
ScAddr FindActByRoleTarget(ScAgentContext & ctx, ScAddr const & targetAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(ScType::ConstNode, ScType::ConstPermPosArc, targetAddr, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(0);
  return ScAddr::Empty;
}

// Все цели заданной роли (rrel_*) из actAddr.
std::vector<ScAddr> RoleTargets(ScAgentContext & ctx, ScAddr const & actAddr, ScAddr const & roleAddr)
{
  std::vector<ScAddr> targets;
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(actAddr, ScType::ConstPermPosArc, ScType::Unknown, ScType::ConstPermPosArc, roleAddr);
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

ScAddr MakeGroup(ScAgentContext & ctx, std::vector<ScAddr> const & members)
{
  ScAddr const groupAddr = ctx.GenerateNode(ScType::ConstNode);
  for (ScAddr const & memberAddr : members)
    ctx.GenerateConnector(ScType::ConstPermPosArc, groupAddr, memberAddr);
  return groupAddr;
}
}  // namespace

// Воспроизводит explanation_77 из scenario_explanation_construction.scs:
// explanandum + explanation_type + непустые uses_results/applies_rules/
// based_on группы + метка времени -> reified concept_explanation_construction_act
// со всеми пятью ролями и nrel_supported_by_explanation.
TEST_F(AgentBuildExplanationTest, BuildsReifiedConstructionActWithAllRoles)
{
  m_ctx->SubscribeAgent<AgentBuildExplanation>();

  ScAddr const explanandumAddr = m_ctx->GenerateNode(ScType::ConstNode);  // обычный processing_result, не integrated
  ScAddr const explanationTypeAddr = m_ctx->GenerateNode(ScType::ConstNode);  // играет роль concept_explanation_contrastive

  ScAddr const partialResult3 = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const usesResultsGroup = MakeGroup(*m_ctx, {partialResult3});

  ScAddr const aggregationRule = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const appliesRulesGroup = MakeGroup(*m_ctx, {aggregationRule});

  ScAddr const ontologyMedicalDiag = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const userHistory = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const basedOnGroup = MakeGroup(*m_ctx, {ontologyMedicalDiag, userHistory});

  ScAddr const constructionTimeLink = m_ctx->GenerateLink(ScType::ConstNodeLink);
  m_ctx->SetLinkContent(constructionTimeLink, std::string("2025-09-12T14:09:14Z"));

  ScAction action = m_ctx->GenerateAction(ExplanationKeynodes::action_build_explanation);
  action.SetArguments(
      explanandumAddr, explanationTypeAddr, usesResultsGroup, appliesRulesGroup, basedOnGroup, constructionTimeLink);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const explanationAddr = TargetOf(*m_ctx, explanandumAddr, ExplanationKeynodes::nrel_supported_by_explanation);
  ASSERT_TRUE(m_ctx->IsElement(explanationAddr));
  EXPECT_TRUE(m_ctx->CheckConnector(explanationTypeAddr, explanationAddr, ScType::ConstPermPosArc));

  // explanandum -- не concept_integrated_result, explained_by не создаётся.
  EXPECT_FALSE(m_ctx->IsElement(TargetOf(*m_ctx, explanandumAddr, ExplanationKeynodes::nrel_explained_by)));

  ScAddr const actAddr =
      FindActByRoleTarget(*m_ctx, explanandumAddr, ExplanationKeynodes::rrel_explanandum);
  ASSERT_TRUE(m_ctx->IsElement(actAddr));
  EXPECT_TRUE(m_ctx->CheckConnector(
      ExplanationKeynodes::concept_explanation_construction_act, actAddr, ScType::ConstPermPosArc));

  std::vector<ScAddr> const typeTargets = RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_explanation_type);
  ASSERT_EQ(typeTargets.size(), 1u);
  EXPECT_EQ(typeTargets[0], explanationTypeAddr);

  EXPECT_TRUE(Contains(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_uses_results), partialResult3));
  EXPECT_TRUE(Contains(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_applies_rules), aggregationRule));
  std::vector<ScAddr> const basedOnTargets = RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_based_on);
  EXPECT_TRUE(Contains(basedOnTargets, ontologyMedicalDiag));
  EXPECT_TRUE(Contains(basedOnTargets, userHistory));

  std::vector<ScAddr> const timeTargets = RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_construction_time);
  ASSERT_EQ(timeTargets.size(), 1u);
  EXPECT_EQ(timeTargets[0], constructionTimeLink);

  ScStructure const resultStruct = action.GetResult();
  EXPECT_TRUE(resultStruct.Has(explanationAddr));
  EXPECT_TRUE(resultStruct.Has(actAddr));

  m_ctx->UnsubscribeAgent<AgentBuildExplanation>();
}

// explanandum, уже классифицированный concept_integrated_result, получает
// и nrel_supported_by_explanation, и nrel_explained_by (как
// integrated_result_19 в scenario_explanation_construction.scs).
// Необязательные группы/метка времени не переданы -- соответствующие
// роли на акте просто отсутствуют, это не ошибка.
TEST_F(AgentBuildExplanationTest, LinksExplainedByOnlyForIntegratedResultAndOmitsMissingRoles)
{
  m_ctx->SubscribeAgent<AgentBuildExplanation>();

  ScAddr const explanandumAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(
      ScType::ConstPermPosArc, ExplanationKeynodes::concept_integrated_result, explanandumAddr);
  ScAddr const explanationTypeAddr = m_ctx->GenerateNode(ScType::ConstNode);  // concept_explanation_global

  ScAction action = m_ctx->GenerateAction(ExplanationKeynodes::action_build_explanation);
  action.SetArguments(explanandumAddr, explanationTypeAddr);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const explanationAddr = TargetOf(*m_ctx, explanandumAddr, ExplanationKeynodes::nrel_supported_by_explanation);
  ASSERT_TRUE(m_ctx->IsElement(explanationAddr));
  EXPECT_EQ(TargetOf(*m_ctx, explanandumAddr, ExplanationKeynodes::nrel_explained_by), explanationAddr);

  ScAddr const actAddr = FindActByRoleTarget(*m_ctx, explanandumAddr, ExplanationKeynodes::rrel_explanandum);
  ASSERT_TRUE(m_ctx->IsElement(actAddr));
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_uses_results).empty());
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_applies_rules).empty());
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_based_on).empty());
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_construction_time).empty());

  m_ctx->UnsubscribeAgent<AgentBuildExplanation>();
}

// Группы переданы, но пусты -- как и с AgentComputePiMetric, "нет
// элементов" не создаёт ролевых связей и не считается ошибкой.
TEST_F(AgentBuildExplanationTest, DoesNotCreateRoleLinksForEmptyGroups)
{
  m_ctx->SubscribeAgent<AgentBuildExplanation>();

  ScAddr const explanandumAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationTypeAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const emptyUsesResults = MakeGroup(*m_ctx, {});
  ScAddr const emptyAppliesRules = MakeGroup(*m_ctx, {});
  ScAddr const emptyBasedOn = MakeGroup(*m_ctx, {});

  ScAction action = m_ctx->GenerateAction(ExplanationKeynodes::action_build_explanation);
  action.SetArguments(explanandumAddr, explanationTypeAddr, emptyUsesResults, emptyAppliesRules, emptyBasedOn);
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedSuccessfully());

  ScAddr const actAddr = FindActByRoleTarget(*m_ctx, explanandumAddr, ExplanationKeynodes::rrel_explanandum);
  ASSERT_TRUE(m_ctx->IsElement(actAddr));
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_uses_results).empty());
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_applies_rules).empty());
  EXPECT_TRUE(RoleTargets(*m_ctx, actAddr, ExplanationKeynodes::rrel_based_on).empty());

  m_ctx->UnsubscribeAgent<AgentBuildExplanation>();
}

// Проверяет, что действие без обязательных аргументов (explanandum,
// explanation_type) завершается с ошибкой, а не молча.
TEST_F(AgentBuildExplanationTest, FinishesWithErrorOnMissingRequiredArguments)
{
  m_ctx->SubscribeAgent<AgentBuildExplanation>();

  ScAction action = m_ctx->GenerateAction(ExplanationKeynodes::action_build_explanation);
  action.SetArguments();
  EXPECT_TRUE(action.InitiateAndWait(2000));
  EXPECT_TRUE(action.IsFinishedWithError());

  m_ctx->UnsubscribeAgent<AgentBuildExplanation>();
}
