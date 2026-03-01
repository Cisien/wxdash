// TDD RED PHASE STUB — tests must fail before implementation
#include "network/JsonParser.h"

namespace JsonParser {

double rainSizeToInches(int /*rainSize*/)
{
    return 0.0; // stub — always returns 0
}

ParsedConditions parseCurrentConditions(const QByteArray & /*data*/)
{
    return {}; // stub — always returns all-nullopt
}

std::optional<UdpReading> parseUdpDatagram(const QByteArray & /*data*/)
{
    return std::nullopt; // stub — always returns nullopt
}

} // namespace JsonParser
