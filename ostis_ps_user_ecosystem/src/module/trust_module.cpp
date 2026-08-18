#include "trust_module.hpp"

#include "agent/agent_update_trust_reliability.hpp"
#include "agent/agent_update_trust_accuracy.hpp"
#include "agent/agent_update_trust_security.hpp"
#include "agent/agent_update_trust_transparency.hpp"

SC_MODULE_REGISTER(TrustModule)
    ->Agent<AgentUpdateTrustReliability>()
    ->Agent<AgentUpdateTrustAccuracy>()
    ->Agent<AgentUpdateTrustSecurity>()
    ->Agent<AgentUpdateTrustTransparency>();
