#pragma once

#include <string>

#include <sc-memory/sc_agent_context.hpp>

namespace trust_utils
{
// SetLinkContent(addr, float) сериализует сырые байты значения
// (ScStreamMakeReadT), тогда как шаблонный GetLinkContent всегда
// читает содержимое через текстовый istringstream — несовместимая
// пара для числовых типов. Поэтому числовые значения хранятся как
// строки явно, используя проверенный в тестах sc-memory путь
// (SetLinkContent/GetLinkContent со строкой).
inline bool ReadFloatLinkContent(ScAgentContext & context, ScAddr const & linkAddr, float & outValue)
{
  std::string content;
  if (!context.GetLinkContent(linkAddr, content))
    return false;

  try
  {
    outValue = std::stof(content);
  }
  catch (std::exception const &)
  {
    return false;
  }
  return true;
}

inline void WriteFloatLinkContent(ScAgentContext & context, ScAddr const & linkAddr, float value)
{
  context.SetLinkContent(linkAddr, std::to_string(value));
}

inline bool IsUnitInterval(float value)
{
  return value >= 0.0f && value <= 1.0f;
}
}  // namespace trust_utils
