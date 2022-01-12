#include "PresetsServerCommunication.h"

namespace PresetsServerCommunication
{
const juce::URL presetServerURL = juce::URL ("https://preset-sharing-server-f4gzy6tzkq-uc.a.run.app");

juce::String sendServerRequest (CommType type, const juce::String& user, const juce::String& pass, const juce::String& presetName)
{
    juce::StringPairArray responseHeaders;
    int statusCode = 0;
    juce::String responseMessage;

    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", user);
    requestHeaders.set ("pass", pass);

    //    if (type == CommType::AddPreset)
    //        requestHeaders.set ("preset", presetName);
    bool isPOST = type == CommType::add_preset;

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (type).data()).withParameters (requestHeaders);

    responseMessage += "URL: " + requestURL.toString (true) + "\n";

    if (auto inputStream = requestURL.createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (5000) // 5 seconds
                .withNumRedirectsToFollow (5)
                .withStatusCode (&statusCode)
                .withResponseHeaders (&responseHeaders)
                .withHttpRequestCmd (isPOST ? "POST" : "GET")))
    {
        auto streamString = inputStream->readEntireStreamAsString();

        responseMessage += "STATUS: " + juce::String (statusCode) + "\n";
        responseMessage += streamString;
    }
    else
    {
        responseMessage += "Unable to connect!";
    }

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}
} // namespace PresetsServerCommunication
