#include "PresetsServerCommunication.h"

namespace PresetsServerCommunication
{
const juce::URL presetServerURL = juce::URL ("https://preset-sharing-server-f4gzy6tzkq-uc.a.run.app");

juce::String sendServerRequest (CommType type, const juce::String& user, const juce::String& pass)
{
    if (type == CommType::add_preset)
    {
        // use sendAddPresetRequest instead!
        jassertfalse;
        return String();
    }

    juce::StringPairArray responseHeaders;
    int statusCode = 0;
    juce::String responseMessage;

    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", user);
    requestHeaders.set ("pass", pass);

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (type).data()).withParameters (requestHeaders);
    responseMessage += "URL: " + requestURL.toString (true) + "\n";

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
        responseMessage += streamString;
    }
    else
    {
        responseMessage += "Unable to connect!";
    }

    Logger::writeToLog ("Pinging Presets Server: " + responseMessage);

    return responseMessage;
}
juce::String sendAddPresetRequest (const juce::String& user, const juce::String& pass, const juce::String& presetName, const String& presetData)
{
    juce::StringPairArray responseHeaders;
    int statusCode = 0;
    juce::String responseMessage;

    juce::StringPairArray requestHeaders;
    requestHeaders.set ("user", user);
    requestHeaders.set ("pass", pass);
    requestHeaders.set ("preset", presetName);

    auto requestURL = presetServerURL.getChildURL (magic_enum::enum_name (CommType::add_preset).data())
                          .withParameters (requestHeaders)
                          .withPOSTData (presetData);
    responseMessage += "URL: " + requestURL.toString (true) + "\n";

    if (auto inputStream = requestURL.createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs (5000) // 5 seconds
                .withNumRedirectsToFollow (5)
                .withStatusCode (&statusCode)
                .withResponseHeaders (&responseHeaders)
                .withHttpRequestCmd ("POST")))
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
