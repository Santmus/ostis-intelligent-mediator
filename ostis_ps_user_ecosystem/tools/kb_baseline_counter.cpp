// Standalone build-time helper -- NOT part of TrustModule and NOT run by
// CTest directly. Links only sc-machine::sc-memory, none of this project's
// src/keynodes/*.hpp -- see the CMakeLists.txt comment above
// kb_baseline_counter's target definition and tests/test_load_real_kb.cpp's
// file header for the full rationale.
//
// Loads a pre-built sc-memory storage (produced by sc-builder from
// ostis_kb_user_ecosystem sources) WITHOUT clearing it, counts every
// sc-element via ScMemoryContext::CalculateStatistics(), and writes a small
// generated header recording that count as the "baseline" element count --
// i.e. the KB's size with only the sc-machine built-in ScKeynodes resolved,
// before any of this project's own keynote classes (TrustKeynodes,
// ProvenanceKeynodes, ...) get a chance to auto-create anything missing.
//
// Never mutates the storage it reads: initializes with clear=SC_FALSE and
// shuts down with saveState=SC_FALSE, the same discipline every test in
// this project follows (see sc-memory/test/sc_test.hpp).
//
// Usage: kb_baseline_counter <storage_dir> <output_header_path>
#include <fstream>
#include <iostream>
#include <string>

#include <sc-memory/sc_memory.hpp>

int main(int argc, char ** argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: kb_baseline_counter <storage_dir> <output_header_path>" << std::endl;
    return 1;
  }
  std::string const storageDir = argv[1];
  std::string const outputHeaderPath = argv[2];

  sc_memory_params params;
  sc_memory_params_clear(&params);
  params.clear = SC_FALSE;
  params.storage = storageDir.c_str();
  params.dump_memory = SC_FALSE;
  params.dump_memory_statistics = SC_FALSE;
  params.log_level = "Error";

  ScMemory::LogMute();
  bool const initialized = ScMemory::Initialize(params);
  ScMemory::LogUnmute();
  if (!initialized)
  {
    std::cerr << "kb_baseline_counter: failed to initialize sc-memory at " << storageDir << std::endl;
    return 1;
  }

  sc_uint64 total = 0;
  {
    ScMemoryContext context;
    total = context.CalculateStatistics().GetAllNum();
    context.Destroy();
  }

  ScMemory::LogMute();
  ScMemory::Shutdown(SC_FALSE);
  ScMemory::LogUnmute();

  std::ofstream out(outputHeaderPath, std::ios::trunc);
  if (!out)
  {
    std::cerr << "kb_baseline_counter: failed to write " << outputHeaderPath << std::endl;
    return 1;
  }
  out << "#pragma once\n";
  out << "#define KB_TEST_REPO_AVAILABLE 1\n";
  out << "#define KB_TEST_REPO_DIR \"" << storageDir << "\"\n";
  out << "#define KB_TEST_REPO_BASELINE_ELEMENT_COUNT " << total << "ULL\n";

  return 0;
}
