#!/bin/bash

class_name=$1
proc_type=$2

proc_name=$class_name
if [ "$#" -eq 3 ]; then
    proc_name=$3
fi

proc_type_upper="$(tr '[:lower:]' '[:upper:]' <<< ${proc_type:0:1})${proc_type:1}"

cat <<EOT > "src/processors/${proc_type}/${class_name}.h"
#pragma once

#include "../BaseProcessor.h"

class ${class_name} : public BaseProcessor
{
public:
    ${class_name} (UndoManager* um = nullptr);

    ProcessorType getProcessorType() const override { return ${proc_type_upper}; }
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (${class_name})
};
EOT

cat <<EOT > "src/processors/${proc_type}/${class_name}.cpp"
#include "${class_name}.h"
#include "../ParameterHelpers.h"

${class_name}::${class_name} (UndoManager* um) : BaseProcessor ("${proc_name}", createParameterLayout(), um)
{
}

AudioProcessorValueTreeState::ParameterLayout ${class_name}::createParameterLayout()
{
    using namespace ParameterHelpers;
    auto params = createBaseParams();

    return { params.begin(), params.end() };
}

void ${class_name}::prepare (double sampleRate, int samplesPerBlock)
{
}

void ${class_name}::processAudio (AudioBuffer<float>& buffer)
{
}
EOT
