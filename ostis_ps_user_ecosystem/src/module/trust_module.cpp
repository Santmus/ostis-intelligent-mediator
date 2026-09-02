#include "trust_module.hpp"

#include "agent/agent_update_trust_reliability.hpp"
#include "agent/agent_update_trust_accuracy.hpp"
#include "agent/agent_update_trust_security.hpp"
#include "agent/agent_update_trust_transparency.hpp"
#include "agent/agent_aggregate_trust.hpp"
#include "agent/agent_compute_pi_metric.hpp"
#include "agent/agent_detect_anomalous_request_rate.hpp"
#include "agent/agent_resolve_by_trust_weighted.hpp"
#include "agent/agent_dispatch_ready_subtask.hpp"
#include "agent/agent_validate_cardinality_constraint.hpp"
#include "agent/agent_register_partial_result.hpp"
#include "agent/agent_propagate_to_dependent.hpp"
#include "agent/agent_build_explanation.hpp"
#include "agent/agent_validate_integration.hpp"
#include "agent/agent_validate_provenance_chain.hpp"
#include "agent/agent_exclude_from_coalition.hpp"
#include "agent/agent_reassign_on_resource_exhaustion.hpp"
#include "agent/agent_check_acyclicity.hpp"
#include "agent/agent_validate_right_to_explanation.hpp"
#include "agent/agent_evaluate_progressive_disclosure.hpp"
#include "agent/agent_choose_modality.hpp"
#include "agent/agent_record_prov_entity.hpp"
#include "agent/agent_record_prov_activity.hpp"
#include "agent/agent_handle_no_admissible_assignment.hpp"

// AgentComputePiMetric/AgentValidateProvenanceChain/AgentRecordProvEntity/
// AgentRecordProvActivity (провенанс),
// AgentDetectAnomalousRequestRate/AgentValidateRightToExplanation
// (безопасность), AgentResolveByTrustWeighted/AgentValidateIntegration
// (интеграция результатов), AgentDispatchReadySubtask/AgentRegisterPartialResult/
// AgentPropagateToDependent (кооперативное исполнение),
// AgentValidateCardinalityConstraint (валидация целостности),
// AgentBuildExplanation (объяснения), AgentExcludeFromCoalition/
// AgentReassignOnResourceExhaustion (реконфигурация коалиции),
// AgentCheckAcyclicity (декомпозиция запроса),
// AgentEvaluateProgressiveDisclosure/AgentChooseModality (адаптация под
// пользователя) и AgentHandleNoAdmissibleAssignment (распределение
// подзадач) не о доверии, но зарегистрированы здесь же:
// SC_MODULE_REGISTER определяет
// extern "C" sc_module_initialize/sc_module_shutdown -- в одной
// разделяемой библиотеке (.dylib/.so) может быть только один модуль,
// иначе конфликт символов. Пока агентов немного, отдельная библиотека
// не оправдана.
SC_MODULE_REGISTER(TrustModule)
    ->Agent<AgentUpdateTrustReliability>()
    ->Agent<AgentUpdateTrustAccuracy>()
    ->Agent<AgentUpdateTrustSecurity>()
    ->Agent<AgentUpdateTrustTransparency>()
    ->Agent<AgentAggregateTrust>()
    ->Agent<AgentComputePiMetric>()
    ->Agent<AgentDetectAnomalousRequestRate>()
    ->Agent<AgentResolveByTrustWeighted>()
    ->Agent<AgentDispatchReadySubtask>()
    ->Agent<AgentValidateCardinalityConstraint>()
    ->Agent<AgentRegisterPartialResult>()
    ->Agent<AgentPropagateToDependent>()
    ->Agent<AgentBuildExplanation>()
    ->Agent<AgentValidateIntegration>()
    ->Agent<AgentValidateProvenanceChain>()
    ->Agent<AgentExcludeFromCoalition>()
    ->Agent<AgentReassignOnResourceExhaustion>()
    ->Agent<AgentCheckAcyclicity>()
    ->Agent<AgentValidateRightToExplanation>()
    ->Agent<AgentEvaluateProgressiveDisclosure>()
    ->Agent<AgentChooseModality>()
    ->Agent<AgentRecordProvEntity>()
    ->Agent<AgentRecordProvActivity>()
    ->Agent<AgentHandleNoAdmissibleAssignment>();
