#include "PresetsServerCommunication.h"

namespace PresetsServerCommunication
{
#if BYOD_USE_LOCAL_PRESET_SERVER
const juce::URL presetServerURL = juce::URL ("http://localhost:8080");
#else
const juce::URL presetServerURL = juce::URL ("https://preset-sharing-server-485ih.ondigitalocean.app");
#endif

String makeMessageString (const String& message)
{
    return "Message: " + message + "\n";
}

String pingServer (const URL& requestURL, const String& httpRequest = "GET")
{
    String responseMessage = "URL: " + requestURL.toString (true) + "\n";

    juce::StringPairArray responseHeaders;
    int statusCode = 0;
    if (auto inputStream = requestURL.createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (5000) // 5 seconds
                .withNumRedirectsToFollow (5)
                .withStatusCode (&statusCode)
                .withResponseHeaders (&responseHeaders)
                .withHttpRequestCmd (httpRequest)))
    {
        auto streamString = inputStream->readEntireStreamAsString();

        responseMessage += "STATUS: " + juce::String (statusCode) + "\n";
        responseMessage += makeMessageString (streamString);
    }
    else
    {
        responseMessage += makeMessageString ("Unable to connect!");
    }

    return responseMessage;
}

juce::String sendServerRequest (CommType type, const juce::String& user, const juce::String& pass)
{
    if (type == CommType::add_preset)
    {
        // use sendAddPresetRequest instead!
        jassertfalse;
        return String();
    }

    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", user);
    requestHeaders.set ("pass", pass);

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (type).data()).withParameters (requestHeaders);
    auto responseMessage = pingServer (requestURL);

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}

juce::String sendAddPresetRequest (const PresetRequestInfo& info)
{
    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", info.user);
    requestHeaders.set ("pass", info.pass);
    requestHeaders.set ("preset", info.presetName);
    requestHeaders.set ("is_public", info.isPublic ? "TRUE" : "FALSE");

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (CommType::add_preset).data())
                          .withParameters (requestHeaders)
                          .withPOSTData (info.presetData);
    auto responseMessage = pingServer (requestURL, "POST");

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}

juce::String sendUpdatePresetRequest (const PresetRequestInfo& info)
{
    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", info.user);
    requestHeaders.set ("pass", info.pass);
    requestHeaders.set ("preset", info.presetName);
    requestHeaders.set ("is_public", info.isPublic ? "TRUE" : "FALSE");
    requestHeaders.set ("preset_id", info.presetID);

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (CommType::update_preset).data())
                          .withParameters (requestHeaders)
                          .withPOSTData (info.presetData);
    auto responseMessage = pingServer (requestURL, "POST");

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}

juce::String parseMessageResponse (const String& messageResponse)
{
    return messageResponse.fromLastOccurrenceOf ("Message: ", false, false).upToLastOccurrenceOf ("\n", false, false);
}

} // namespace PresetsServerCommunication
