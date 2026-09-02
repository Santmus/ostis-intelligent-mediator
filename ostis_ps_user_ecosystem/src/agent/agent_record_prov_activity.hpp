#pragma once

#include <sc-memory/sc_agent.hpp>

// Реализует rule_auto_record_prov_activity (rules/rules_provenance_completion.scs,
// agent_provenance_registrar): для выполнения процесса создаёт PROV-O
// деятельность a (concept_prov_activity) с ролями rrel_actor/rrel_input/
// rrel_timestamp, связывает её с использованным входом через PROV:used
// и с исполнившим PROV-агентом через wasAssociatedWith.
//
// Аргументы sc-действия action_record_prov_activity:
//   1) actor       -- s_a, цель роли rrel_actor (агент доверяет
//                      вызывающей стороне относительно класса, как и
//                      везде в модуле; rrel_actor формально объявлен
//                      как concept_interaction_act -> concept_actor
//                      (relations/role_relations.scs), a -- не
//                      concept_interaction_act -- та же практика
//                      "наблюдение, не исправление", что и у
//                      AgentValidateProvenanceChain/nrel_derived_from)
//   2) input       -- in_i, concept_prov_entity, использованный вход;
//                      цель роли rrel_input И цель PROV:used(a, in_i)
//   3) prov_agent  -- concept_prov_agent, соответствующий s_a; правило
//                      называет его agent_of(s_a), но функция agent_of
//                      нигде не формализована формулой -- вызывающая
//                      сторона передаёт уже готовый concept_prov_agent
//                      отдельным аргументом, а не выводится агентом
//   4) timestamp   -- t, ОПЦИОНАЛЬНЫЙ sc-ссылка для роли rrel_timestamp
//                      (тот же приём, что construction_time у
//                      AgentBuildExplanation -- последний аргумент,
//                      непустой слот проверяется IsElement)
//
// "When: sc-агент s_a выполняет процесс π_i" не реализовано как
// автоматический триггер -- вызывающая сторона инициирует регистрацию
// явно, как и везде в модуле.
//
// Не реализовано:
//  - отображение actor -> prov_agent (agent_of(s_a)) не вычисляется --
//    оба передаются отдельными аргументами;
//  - только IsElement на обязательных аргументах, как и у остальных
//    агентов модуля.
class AgentRecordProvActivity : public ScActionInitiatedAgent
{
public:
  ScAddr GetActionClass() const override;

  ScResult DoProgram(ScAction & action) override;
};
