#include "trust_module.hpp"

#include "agent/agent_update_trust_reliability.hpp"
#include "agent/agent_update_trust_accuracy.hpp"
#include "agent/agent_update_trust_security.hpp"
#include "agent/agent_update_trust_transparency.hpp"
#include "agent/agent_aggregate_trust.hpp"
#include "agent/agent_compute_pi_metric.hpp"
#include "agent/agent_detect_anomalous_request_rate.hpp"
#include "agent/agent_resolve_by_trust_weighted.hpp"

// AgentComputePiMetric (провенансные метрики), AgentDetectAnomalousRequestRate
// (безопасность) и AgentResolveByTrustWeighted (интеграция результатов)
// не о доверии, но зарегистрированы здесь же: SC_MODULE_REGISTER
// определяет extern "C" sc_module_initialize/sc_module_shutdown -- в
// одной разделяемой библиотеке (.dylib/.so) может быть только один
// модуль, иначе конфликт символов. Пока агентов немного, отдельная
// библиотека не оправдана.
SC_MODULE_REGISTER(TrustModule)
    ->Agent<AgentUpdateTrustReliability>()
    ->Agent<AgentUpdateTrustAccuracy>()
    ->Agent<AgentUpdateTrustSecurity>()
    ->Agent<AgentUpdateTrustTransparency>()
    ->Agent<AgentAggregateTrust>()
    ->Agent<AgentComputePiMetric>()
    ->Agent<AgentDetectAnomalousRequestRate>()
    ->Agent<AgentResolveByTrustWeighted>();
