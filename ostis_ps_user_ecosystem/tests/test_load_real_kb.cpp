// Loads the real ostis_kb_user_ecosystem KB and verifies that NONE of this
// project's sc-keynotes (every static ScKeynode declared across
// src/keynodes/*.hpp -- TrustKeynodes, ProvenanceKeynodes, SecurityKeynodes,
// IntegrationKeynodes, TaskKeynodes, IntegrityKeynodes, ExplanationKeynodes,
// CoalitionKeynodes, AdaptiveInterfaceKeynodes, AllocationKeynodes) gets
// silently auto-created rather than resolved against an already-existing KB
// element -- AGENTS_AUDIT.md §7.7, item 1 ("a test that loads the real KB
// and resolves every keynode").
//
// Why a plain IsElement()/SearchElementBySystemIdentifier() check cannot do
// this: ScKeynode's own resolution (sc-memory/sc_keynodes.cpp,
// ScKeynode::Initialize -> ScMemoryContext::ResolveElementSystemIdentifier)
// is find-OR-CREATE, not find-or-fail -- a missing identifier is silently
// created, not reported. That resolution happens unconditionally inside
// ScMemory::Initialize() (which calls ScKeynodes::Initialize() directly,
// see sc_memory.cpp), before any test code gets control, for every
// ScKeynode ever statically constructed in the process -- project-specific
// ones included, since they all share one process-global registration list
// (internal::ScKeynodesRegister). So by the time this test's SetUp() can
// call anything, every keynote this binary declares has already been
// resolved one way or the other; searching for them again would always
// succeed regardless of whether they pre-existed.
//
// The only reliable signal left is the KB's total element count. This test
// compares it, right after loading the real KB with the full TrustModule
// linked in (registering every project keynote for auto-resolution),
// against a baseline captured by tools/kb_baseline_counter.cpp: a separate,
// minimal executable that links ONLY sc-machine::sc-memory (none of this
// project's keynote headers) against the exact same pre-built storage, so
// its own count reflects only the sc-machine built-in ScKeynodes plus
// whatever the real KB already contains. Any project keynote that does not
// already exist in the real KB inflates this test's count beyond that
// baseline, and the test fails.
//
// The KB copy loaded here (KB_TEST_REPO_DIR, from the generated
// kb_test_repo_config.hpp -- see CMakeLists.txt) lives under the build
// tree, NOT the production ostis_ps_user_ecosystem/repo/ that sc-server
// actually serves, and is loaded with clear=SC_FALSE / shut down with
// saveState=SC_FALSE, so this test never mutates or erases anything outside
// its own private, disposable copy.
//
// Coverage caveat: this only catches drift in a keynote that is
// odr-used somewhere in this project's linked code (every keynote
// currently is, since each one exists because some agent reads it) --
// a hypothetical keynote declared but never referenced anywhere could, in
// principle, be optimized away before ScKeynode's constructor runs. None
// of this project's keynotes are currently in that position.
#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_memory.hpp>

#include "kb_test_repo_config.hpp"

namespace
{
class LoadRealKbTest : public testing::Test
{
protected:
  void SetUp() override
  {
#if !KB_TEST_REPO_AVAILABLE
    GTEST_SKIP() << "sc-builder was not found at CMake configure time -- see the warning "
                     "printed by ostis_ps_user_ecosystem/CMakeLists.txt";
#else
    sc_memory_params params;
    sc_memory_params_clear(&params);
    params.clear = SC_FALSE;  // load the pre-built copy, never wipe it
    params.storage = KB_TEST_REPO_DIR;
    params.dump_memory = SC_FALSE;
    params.dump_memory_statistics = SC_FALSE;
    params.log_level = "Error";

    ScMemory::LogMute();
    bool const initialized = ScMemory::Initialize(params);
    ScMemory::LogUnmute();
    ASSERT_TRUE(initialized) << "failed to load the isolated KB test copy at " << KB_TEST_REPO_DIR;

    m_ctx = std::make_unique<ScMemoryContext>();
#endif
  }

  void TearDown() override
  {
    if (m_ctx)
    {
      m_ctx->Destroy();
      ScMemory::LogMute();
      ScMemory::Shutdown(SC_FALSE);  // never persist -- see file header
      ScMemory::LogUnmute();
    }
  }

  std::unique_ptr<ScMemoryContext> m_ctx;
};
}  // namespace

TEST_F(LoadRealKbTest, EveryProjectKeynodeResolvesAgainstExistingKbElements)
{
  ScMemoryContext::ScMemoryStatistics const stats = m_ctx->CalculateStatistics();
  EXPECT_EQ(stats.GetAllNum(), KB_TEST_REPO_BASELINE_ELEMENT_COUNT)
      << "one or more project keynodes did not already exist in the real KB and were "
         "silently auto-created instead of resolved -- see this test's file header for why "
         "IsElement()-style checks cannot catch this directly; check every *_keynodes.hpp "
         "identifier under src/keynodes/ against ostis_kb_user_ecosystem for a rename/typo.";
}
