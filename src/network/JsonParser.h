#pragma once

#include "models/WeatherReadings.h"

#include <QByteArray>
#include <optional>

/**
 * Stateless JSON parsing functions for WeatherLink Live API responses.
 *
 * All functions accept raw QByteArray input and return typed structs or
 * std::optional. Malformed JSON or missing fields silently return
 * empty/nullopt — per locked decision, silently ignore what can't be parsed.
 *
 * IMPORTANT: Never rely on array index to identify sensor type.
 * Always check data_structure_type field:
 *   1 = ISS (outdoor temp/hum/wind/rain/solar/UV)
 *   3 = LSS BAR (barometric pressure)
 *   4 = LSS Temp/Hum (indoor temperature and humidity)
 */
namespace JsonParser {

/**
 * Returns the rain count-to-inches multiplier for a given rain_size value.
 *
 * rain_size values per WeatherLink Live API:
 *   1 = 0.01 inches per count
 *   2 = 0.2 mm per count  → 0.2 / 25.4 inches
 *   3 = 0.1 mm per count  → 0.1 / 25.4 inches
 *   4 = 0.001 inches per count
 *
 * Unknown values fall back to 0.01 (rain_size 1 is most common).
 */
double rainSizeToInches(int rainSize);

/**
 * Parsed result from parseCurrentConditions().
 * Each field is populated only if the corresponding sensor type was found.
 */
struct ParsedConditions {
    std::optional<IssReading> iss;
    std::optional<BarReading> bar;
    std::optional<IndoorReading> indoor;
};

/**
 * Parse an HTTP /v1/current_conditions response body.
 *
 * Expected structure:
 *   { "data": { "conditions": [ { "data_structure_type": N, ... }, ... ] } }
 *
 * Returns ParsedConditions with nullopt for any type not present in the response.
 * Returns all-nullopt on malformed JSON or missing fields.
 */
ParsedConditions parseCurrentConditions(const QByteArray &data);

/**
 * Parse a UDP real-time broadcast datagram.
 *
 * Expected structure:
 *   { "conditions": [ { "data_structure_type": 1, ... } ] }
 *
 * Returns the first type-1 entry as a UdpReading, or nullopt if:
 *   - JSON is malformed
 *   - No type-1 entry is present in conditions
 */
std::optional<UdpReading> parseUdpDatagram(const QByteArray &data);

} // namespace JsonParser
