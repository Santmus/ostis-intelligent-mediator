#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_agent.hpp>
#include <sc-memory/sc_action.hpp>

#include "agent/agent_validate_cardinality_constraint.hpp"
#include "agent/agent_update_trust_reliability.hpp"
#include "agent/agent_aggregate_trust.hpp"
#include "agent/agent_resolve_by_trust_weighted.hpp"
#include "agent/agent_dispatch_ready_subtask.hpp"
#include "agent/agent_register_partial_result.hpp"
#include "agent/agent_propagate_to_dependent.hpp"
#include "agent/agent_validate_integration.hpp"
#include "agent/agent_record_state_transition.hpp"
#include "keynodes/integrity_keynodes.hpp"
#include "keynodes/trust_keynodes.hpp"
#include "keynodes/integration_keynodes.hpp"
#include "keynodes/task_keynodes.hpp"
#include "keynodes/lifecycle_keynodes.hpp"

// Не отдельный агент, а сквозной тест, показывающий, что выход одного
// уже реализованного агента корректно читается следующим -- без
// оркестратора (в проекте его пока нет, см. "Сквозные зависимости" в
// agents_vault/00 Индекс.md) вызовы просто идут друг за другом внутри
// одного теста.
using IntegrationRequestPipelineTest = ScMemoryTest;

namespace
{
ScAddr MakeFloatLink(ScAgentContext & ctx, float value)
{
  ScAddr const linkAddr = ctx.GenerateLink(ScType::ConstNodeLink);
  ctx.SetLinkContent(linkAddr, std::to_string(value));
  return linkAddr;
}

void LinkByRelation(ScAgentContext & ctx, ScAddr const & src, ScAddr const & trg, ScAddr const & relationAddr)
{
  ScAddr const arcAddr = ctx.GenerateConnector(ScType::ConstCommonArc, src, trg);
  ctx.GenerateConnector(ScType::ConstPermPosArc, relationAddr, arcAddr);
}

float ReadValueLinkContent(ScAgentContext & ctx, ScAddr const & nodeAddr)
{
  ScIterator3Ptr const it3 = ctx.CreateIterator3(nodeAddr, ScType::ConstPermPosArc, ScType::ConstNodeLink);
  EXPECT_TRUE(it3->Next());
  std::string content;
  EXPECT_TRUE(ctx.GetLinkContent(it3->Get(2), content));
  return std::stof(content);
}

ScAddr TargetOf(ScAgentContext & ctx, ScAddr const & src, ScAddr const & relationAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(src, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, relationAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Цель роли roleAddr в представлении LinkByRole (common/relation_io.hpp):
// sourceAddr -[ConstPermPosArc]-> target, дуга классифицирована roleAddr.
ScAddr RoleTarget(ScAgentContext & ctx, ScAddr const & sourceAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(sourceAddr, ScType::ConstPermPosArc, ScType::Unknown, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(2);
  return ScAddr::Empty;
}

// Обратный поиск: источник, присоединивший targetAddr в роли roleAddr --
// находит узел перехода concept_state_transition по его rrel_to_state.
ScAddr RoleSource(ScAgentContext & ctx, ScAddr const & targetAddr, ScAddr const & roleAddr)
{
  ScIterator5Ptr const it5 =
      ctx.CreateIterator5(ScType::Unknown, ScType::ConstPermPosArc, targetAddr, ScType::ConstPermPosArc, roleAddr);
  if (it5->Next())
    return it5->Get(0);
  return ScAddr::Empty;
}
}  // namespace

// Воспроизводит фрагмент scenario_request_processing.scs: пользователь
// формулирует ровно один запрос (шаг 1, форма shape_user_request_authorship,
// [1,1] -- AgentValidateCardinalityConstraint); принятая с высокой оценкой
// обратная связь (шаг 8, "оценка: пользователь принял рекомендацию, 5/5")
// поднимает надёжность обработавшей запрос системы с нейтрального нуля
// (AgentUpdateTrustReliability); агрегированное доверие (AgentAggregateTrust,
// rule_aggregate_trust) используется затем, чтобы разрешить конфликт двух
// результатов (rule_resolve_by_trust_weighted) в пользу этой системы против
// заведомо менее доверенного конкурента. Цепочка: Validate -> Update ->
// Aggregate -> Resolve -- четыре разных агента, три разных семейства
// (целостность / доверие / интеграция), объединённые общими данными, а не
// новым оркестрирующим агентом.
TEST_F(IntegrationRequestPipelineTest, ValidatesUpdatesAggregatesAndResolvesConflict)
{
  m_ctx->SubscribeAgent<AgentValidateCardinalityConstraint>();
  m_ctx->SubscribeAgent<AgentUpdateTrustReliability>();
  m_ctx->SubscribeAgent<AgentAggregateTrust>();
  m_ctx->SubscribeAgent<AgentResolveByTrustWeighted>();

  // --- Шаг 1: user -[nrel_formulates]-> request, ровно один автор. ---
  ScAddr const nrelFormulatesAddr = m_ctx->GenerateNode(ScType::ConstNodeNonRole);
  ScAddr const userAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, userAddr, requestAddr, nrelFormulatesAddr);

  ScAddr const minCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const maxCountAddr = MakeFloatLink(*m_ctx, 1.0f);
  ScAddr const validationLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction validateAction = m_ctx->GenerateAction(IntegrityKeynodes::action_validate_cardinality_constraint);
  validateAction.SetArguments(
      requestAddr,
      nrelFormulatesAddr,
      IntegrityKeynodes::concept_cardinality_check_incoming_non_role,
      minCountAddr,
      maxCountAddr,
      IntegrityKeynodes::concept_cardinality_constraint_violation,
      validationLevelAddr);
  ASSERT_TRUE(validateAction.InitiateAndWait(2000));
  ASSERT_TRUE(validateAction.IsFinishedSuccessfully());
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, validationLevelAddr), 1.0f, 0.0001f);

  // --- Шаг 2: положительный исход (nu_rel = 1.0, "оценка 5/5") поднимает
  // t_rel обработавшей запрос системы с нейтрального нуля. ---
  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const relLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, relLevelAddr);
  ScAddr const outcomeLinkAddr = MakeFloatLink(*m_ctx, 1.0f);

  ScAction updateAction = m_ctx->GenerateAction(TrustKeynodes::action_update_trust_reliability);
  updateAction.SetArguments(relLevelAddr, outcomeLinkAddr);
  ASSERT_TRUE(updateAction.InitiateAndWait(2000));
  ASSERT_TRUE(updateAction.IsFinishedSuccessfully());
  // (1 - eta) * 0 + eta * 1.0, eta = 0.10 по умолчанию
  // (agent_update_trust_reliability.cpp, аргумент eta_link не передан).
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, relLevelAddr), 0.10f, 0.0001f);

  // --- Шаг 3: агрегируем -- обновлена только t_rel, остальные три
  // компоненты нейтральны (без value_link => 0). ---
  ScAddr const accLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const secLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const trLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const trustLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, trustLevelAddr);

  ScAction aggregateAction = m_ctx->GenerateAction(TrustKeynodes::action_aggregate_trust);
  aggregateAction.SetArguments(relLevelAddr, accLevelAddr, secLevelAddr, trLevelAddr, trustLevelAddr);
  ASSERT_TRUE(aggregateAction.InitiateAndWait(2000));
  ASSERT_TRUE(aggregateAction.IsFinishedSuccessfully());
  // lambda_rel * 0.10, lambda_rel = 0.25 (равные веса, agent_aggregate_trust.cpp);
  // 0.025 > 0.01 -- конфликт ниже по-прежнему разрешается в пользу системы.
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, trustLevelAddr), 0.025f, 0.0001f);

  // Замыкаем цепочку, которую ожидает AgentResolveByTrustWeighted:
  // result -[nrel_produced_by]-> system -[nrel_has_trust_level]-> trust_level.
  LinkByRelation(*m_ctx, systemAddr, trustLevelAddr, IntegrationKeynodes::nrel_has_trust_level);

  ScAddr const resultFromSystemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, resultFromSystemAddr, systemAddr, IntegrationKeynodes::nrel_produced_by);

  // --- Шаг 4: конкурирующий результат от заведомо менее доверенной
  // системы (0.01 < 0.04) -- конфликт разрешается по взвешенному доверию. ---
  ScAddr const competitorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const competitorTrustAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, TrustKeynodes::concept_trust_level, competitorTrustAddr);
  LinkByRelation(*m_ctx, competitorAddr, competitorTrustAddr, IntegrationKeynodes::nrel_has_trust_level);
  ScAddr const competitorValueLinkAddr = MakeFloatLink(*m_ctx, 0.01f);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, competitorTrustAddr, competitorValueLinkAddr);

  ScAddr const resultFromCompetitorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, resultFromCompetitorAddr, competitorAddr, IntegrationKeynodes::nrel_produced_by);

  ScAddr const candidatesGroupAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, candidatesGroupAddr, resultFromSystemAddr);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, candidatesGroupAddr, resultFromCompetitorAddr);
  ScAddr const conflictAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction resolveAction = m_ctx->GenerateAction(IntegrationKeynodes::action_resolve_by_trust_weighted);
  resolveAction.SetArguments(candidatesGroupAddr, conflictAddr);
  ASSERT_TRUE(resolveAction.InitiateAndWait(2000));
  ASSERT_TRUE(resolveAction.IsFinishedSuccessfully());

  ScStructure const resolveResult = resolveAction.GetResult();
  EXPECT_TRUE(resolveResult.Has(resultFromSystemAddr));
  EXPECT_FALSE(resolveResult.Has(resultFromCompetitorAddr));

  m_ctx->UnsubscribeAgent<AgentValidateCardinalityConstraint>();
  m_ctx->UnsubscribeAgent<AgentUpdateTrustReliability>();
  m_ctx->UnsubscribeAgent<AgentAggregateTrust>();
  m_ctx->UnsubscribeAgent<AgentResolveByTrustWeighted>();
}

// Прогоняет полный жизненный цикл agent_cooperative_executor по цепочке
// подзадач σ_dep (уже завершена) -> σ -> σ' из
// scenario_request_processing.scs: σ назначена и готова к запуску,
// поскольку её единственная зависимость σ_dep уже completed
// (AgentDispatchReadySubtask); система присылает результат по σ, что
// регистрирует его как concept_partial_result с провенансным триплетом
// и переводит σ в completed (AgentRegisterPartialResult); наконец, σ',
// зависящая от σ, получает этот результат как вход через inform-акт
// (AgentPropagateToDependent). Три разных агента одного и того же
// agent_cooperative_executor, связанные состоянием одной подзадачи σ, а
// не оркестратором.
TEST_F(IntegrationRequestPipelineTest, DispatchesRegistersAndPropagatesAlongSubtaskChain)
{
  m_ctx->SubscribeAgent<AgentDispatchReadySubtask>();
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();
  m_ctx->SubscribeAgent<AgentPropagateToDependent>();

  // --- Шаг 1: σ назначена системе, статус proposed, единственная
  // зависимость σ_dep уже completed -- готова к диспетчеризации. ---
  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(
      *m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_proposed, TaskKeynodes::nrel_subtask_status);

  ScAddr const dependencyAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(
      *m_ctx,
      dependencyAddr,
      TaskKeynodes::concept_subtask_status_completed,
      TaskKeynodes::nrel_subtask_status);
  LinkByRelation(*m_ctx, subtaskAddr, dependencyAddr, TaskKeynodes::nrel_subtask_depends_on);

  ScAddr const dispatchLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAction dispatchAction = m_ctx->GenerateAction(TaskKeynodes::action_dispatch_ready_subtask);
  dispatchAction.SetArguments(subtaskAddr, dispatchLevelAddr);
  ASSERT_TRUE(dispatchAction.InitiateAndWait(2000));
  ASSERT_TRUE(dispatchAction.IsFinishedSuccessfully());
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, dispatchLevelAddr), 1.0f, 0.0001f);

  // --- Шаг 2: система присылает результат r по σ -- регистрируется
  // провенанс, σ переходит в completed. ---
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction registerAction = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  registerAction.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  ASSERT_TRUE(registerAction.InitiateAndWait(2000));
  ASSERT_TRUE(registerAction.IsFinishedSuccessfully());
  EXPECT_TRUE(m_ctx->CreateIterator3(TaskKeynodes::concept_partial_result, ScType::ConstPermPosArc, resultAddr)
                  ->Next());

  ScIterator5Ptr const statusAfterRegisterIt = m_ctx->CreateIterator5(
      subtaskAddr, ScType::ConstCommonArc, ScType::Unknown, ScType::ConstPermPosArc, TaskKeynodes::nrel_subtask_status);
  ASSERT_TRUE(statusAfterRegisterIt->Next());
  EXPECT_EQ(statusAfterRegisterIt->Get(2), TaskKeynodes::concept_subtask_status_completed);

  // --- Шаг 3: σ' зависит от только что завершённой σ и уже назначена
  // исполнителю -- получает r как вход (is_input_for) и через inform-акт,
  // адресованный исполнителю, а не самой подзадаче (AGENTS_AUDIT.md
  // п.6.10). ---
  ScAddr const dependentSubtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const dependentExecutorAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, dependentSubtaskAddr, subtaskAddr, TaskKeynodes::nrel_subtask_depends_on);
  LinkByRelation(*m_ctx, dependentSubtaskAddr, dependentExecutorAddr, TaskKeynodes::nrel_assigned_to);

  ScAddr const propagateLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAction propagateAction = m_ctx->GenerateAction(TaskKeynodes::action_propagate_to_dependent);
  propagateAction.SetArguments(resultAddr, subtaskAddr, propagateLevelAddr);
  ASSERT_TRUE(propagateAction.InitiateAndWait(2000));
  ASSERT_TRUE(propagateAction.IsFinishedSuccessfully());
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, propagateLevelAddr), 1.0f, 0.0001f);

  ScIterator5Ptr const isInputForIt = m_ctx->CreateIterator5(
      resultAddr,
      ScType::ConstCommonArc,
      dependentSubtaskAddr,
      ScType::ConstPermPosArc,
      TaskKeynodes::nrel_is_input_for);
  EXPECT_TRUE(isInputForIt->Next());

  ScIterator5Ptr const informIt = m_ctx->CreateIterator5(
      ScType::ConstNode,
      ScType::ConstPermPosArc,
      dependentExecutorAddr,
      ScType::ConstPermPosArc,
      TaskKeynodes::rrel_receiver);
  ASSERT_TRUE(informIt->Next());
  ScAddr const informActAddr = informIt->Get(0);
  EXPECT_TRUE(m_ctx->CreateIterator3(informActAddr, ScType::ConstPermPosArc, resultAddr)->Next());

  m_ctx->UnsubscribeAgent<AgentDispatchReadySubtask>();
  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
  m_ctx->UnsubscribeAgent<AgentPropagateToDependent>();
}

// Расширяет тот же участок конвейера учётом состояния запроса
// (AGENTS_AUDIT.md §7.1, Фаза 1): после того как σ получает результат
// (тот же фрагмент, что и в предыдущем тесте), запрос переходит
// x_exec -> x_aggregate (AgentRecordStateTransition); интегрированный
// результат проходит проверку (AgentValidateIntegration,
// rule_validate_integration); затем запрос переходит x_aggregate ->
// x_verify. Обход rrel_to_state/rrel_from_state двух узлов перехода
// восстанавливает x_exec -> x_aggregate -> x_verify
// (affirmation_transition_traceability) -- без единого оркестрирующего
// агента, той же дисциплиной, что и в двух тестах выше.
TEST_F(IntegrationRequestPipelineTest, RecordsStateTransitionsAroundIntegrationValidation)
{
  m_ctx->SubscribeAgent<AgentDispatchReadySubtask>();
  m_ctx->SubscribeAgent<AgentRegisterPartialResult>();
  m_ctx->SubscribeAgent<AgentRecordStateTransition>();
  m_ctx->SubscribeAgent<AgentValidateIntegration>();

  // --- Шаг 1: σ назначена, единственная зависимость уже завершена --
  //     готова к диспетчеризации (тот же фрагмент, что и выше). ---
  ScAddr const systemAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const subtaskAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, subtaskAddr, systemAddr, TaskKeynodes::nrel_assigned_to);
  LinkByRelation(
      *m_ctx, subtaskAddr, TaskKeynodes::concept_subtask_status_proposed, TaskKeynodes::nrel_subtask_status);

  ScAddr const dependencyAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(
      *m_ctx, dependencyAddr, TaskKeynodes::concept_subtask_status_completed, TaskKeynodes::nrel_subtask_status);
  LinkByRelation(*m_ctx, subtaskAddr, dependencyAddr, TaskKeynodes::nrel_subtask_depends_on);

  ScAddr const dispatchLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAction dispatchAction = m_ctx->GenerateAction(TaskKeynodes::action_dispatch_ready_subtask);
  dispatchAction.SetArguments(subtaskAddr, dispatchLevelAddr);
  ASSERT_TRUE(dispatchAction.InitiateAndWait(2000));
  ASSERT_TRUE(dispatchAction.IsFinishedSuccessfully());

  // --- Шаг 2: система присылает результат -- σ завершена. ---
  ScAddr const resultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const contextAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAction registerAction = m_ctx->GenerateAction(TaskKeynodes::action_register_partial_result);
  registerAction.SetArguments(systemAddr, resultAddr, subtaskAddr, contextAddr);
  ASSERT_TRUE(registerAction.InitiateAndWait(2000));
  ASSERT_TRUE(registerAction.IsFinishedSuccessfully());

  // --- Шаг 3: все результаты собраны -- запрос переходит x_exec -> x_aggregate. ---
  ScAddr const requestAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const execStateAddr = m_ctx->GenerateNode(ScType::ConstNode);
  m_ctx->GenerateConnector(ScType::ConstPermPosArc, LifecycleKeynodes::concept_state_execution, execStateAddr);
  LinkByRelation(
      *m_ctx, execStateAddr, MakeFloatLink(*m_ctx, 2.0f), LifecycleKeynodes::nrel_remaining_reassignment_budget);
  LinkByRelation(
      *m_ctx, execStateAddr, MakeFloatLink(*m_ctx, 1.0f), LifecycleKeynodes::nrel_remaining_replanning_depth);
  LinkByRelation(*m_ctx, requestAddr, execStateAddr, LifecycleKeynodes::nrel_has_state);
  LinkByRelation(
      *m_ctx, LifecycleKeynodes::concept_state_execution, LifecycleKeynodes::concept_state_aggregation,
      LifecycleKeynodes::nrel_transitions_to);

  ScAddr const allResultsReadyEventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const planVersionAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction toAggregateAction = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  toAggregateAction.SetArguments(
      requestAddr, LifecycleKeynodes::concept_state_aggregation, allResultsReadyEventAddr, planVersionAddr);
  ASSERT_TRUE(toAggregateAction.InitiateAndWait(2000));
  ASSERT_TRUE(toAggregateAction.IsFinishedSuccessfully());

  ScAddr const aggregateStateAddr = TargetOf(*m_ctx, requestAddr, LifecycleKeynodes::nrel_has_state);
  ASSERT_TRUE(m_ctx->IsElement(aggregateStateAddr));

  // --- Шаг 4: результат интегрирован и объяснён, конфликтов нет --
  //     rule_validate_integration подтверждает валидность (типизация
  //     concept_integrated_result агентом не проверяется, см.
  //     agent_validate_integration.hpp). ---
  ScAddr const integratedResultAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAddr const explanationAddr = m_ctx->GenerateNode(ScType::ConstNode);
  LinkByRelation(*m_ctx, integratedResultAddr, explanationAddr, IntegrationKeynodes::nrel_explained_by);
  ScAddr const validationLevelAddr = m_ctx->GenerateNode(ScType::ConstNode);

  ScAction validateIntegrationAction = m_ctx->GenerateAction(IntegrationKeynodes::action_validate_integration);
  validateIntegrationAction.SetArguments(integratedResultAddr, validationLevelAddr);
  ASSERT_TRUE(validateIntegrationAction.InitiateAndWait(2000));
  ASSERT_TRUE(validateIntegrationAction.IsFinishedSuccessfully());
  EXPECT_NEAR(ReadValueLinkContent(*m_ctx, validationLevelAddr), 1.0f, 0.0001f);

  // --- Шаг 5: интеграция подтверждена -- запрос переходит x_aggregate -> x_verify. ---
  LinkByRelation(
      *m_ctx, LifecycleKeynodes::concept_state_aggregation, LifecycleKeynodes::concept_state_verification,
      LifecycleKeynodes::nrel_transitions_to);

  ScAddr const integrationValidEventAddr = m_ctx->GenerateNode(ScType::ConstNode);
  ScAction toVerifyAction = m_ctx->GenerateAction(LifecycleKeynodes::action_record_state_transition);
  toVerifyAction.SetArguments(
      requestAddr, LifecycleKeynodes::concept_state_verification, integrationValidEventAddr, planVersionAddr);
  ASSERT_TRUE(toVerifyAction.InitiateAndWait(2000));
  ASSERT_TRUE(toVerifyAction.IsFinishedSuccessfully());

  ScAddr const verifyStateAddr = TargetOf(*m_ctx, requestAddr, LifecycleKeynodes::nrel_has_state);
  ASSERT_TRUE(m_ctx->IsElement(verifyStateAddr));
  EXPECT_TRUE(
      m_ctx
          ->CreateIterator3(LifecycleKeynodes::concept_state_verification, ScType::ConstPermPosArc, verifyStateAddr)
          ->Next());

  // --- affirmation_transition_traceability: обход rrel_to_state/
  //     rrel_from_state двух узлов перехода восстанавливает
  //     x_exec -> x_aggregate -> x_verify. ---
  ScAddr const transitionToVerifyAddr = RoleSource(*m_ctx, verifyStateAddr, LifecycleKeynodes::rrel_to_state);
  ASSERT_TRUE(m_ctx->IsElement(transitionToVerifyAddr));
  EXPECT_EQ(RoleTarget(*m_ctx, transitionToVerifyAddr, LifecycleKeynodes::rrel_from_state), aggregateStateAddr);

  ScAddr const transitionToAggregateAddr = RoleSource(*m_ctx, aggregateStateAddr, LifecycleKeynodes::rrel_to_state);
  ASSERT_TRUE(m_ctx->IsElement(transitionToAggregateAddr));
  EXPECT_EQ(RoleTarget(*m_ctx, transitionToAggregateAddr, LifecycleKeynodes::rrel_from_state), execStateAddr);

  m_ctx->UnsubscribeAgent<AgentDispatchReadySubtask>();
  m_ctx->UnsubscribeAgent<AgentRegisterPartialResult>();
  m_ctx->UnsubscribeAgent<AgentRecordStateTransition>();
  m_ctx->UnsubscribeAgent<AgentValidateIntegration>();
}
