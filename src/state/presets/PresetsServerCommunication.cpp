#include "PresetsServerCommunication.h"

namespace PresetsServerCommunication
{
//const juce::URL presetServerURL = juce::URL ("https://preset-sharing-server-f4gzy6tzkq-uc.a.run.app");
const juce::URL presetServerURL = juce::URL ("http://localhost:8080");

String makeMessageString (const String& message)
{
    return "Message: " + message + "\n";
}

String pingServer (const URL& requestURL)
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
                .withHttpRequestCmd ("GET")))
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
    const auto responseMessage = pingServer (requestURL);

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}

juce::String sendAddPresetRequest (const juce::String& user, const juce::String& pass, const juce::String& presetName, const String& presetData)
{
    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", user);
    requestHeaders.set ("pass", pass);
    requestHeaders.set ("preset", presetName);

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (CommType::add_preset).data())
                          .withParameters (requestHeaders)
                          .withPOSTData (presetData);
    const auto responseMessage = pingServer (requestURL);

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}

juce::String parseMessageResponse (const String& messageResponse)
{
    return messageResponse.fromLastOccurrenceOf ("Message: ", false, false).upToLastOccurrenceOf ("\n", false, false);
}

} // namespace PresetsServerCommunication
